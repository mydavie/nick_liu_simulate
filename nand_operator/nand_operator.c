#include "nand_operator.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <bson.h>
#include <stdio.h>
#include <locale.h>
#include <memory.h>
#include <assert.h>
#include <mongoc/mongoc.h>
#include "../utility/types.h"
#include "../utility/utility.h"
#include "../utility/pool_mgr.h"
#include "../simulator/mongodb_operator.h"
#include "nand_operator.h"

nand_vector_t l2p_lun_table[MAX_CHANNEL_NR + MAX_CE_PER_CHANNEL + MAX_LUN_PER_CE];

nand_info_t gnand_info;
nand_mgr_t  gnand_mgr;

//the info will get from NAND parameter page.
uint32 nand_info_initialization(nand_info_t *pnand_info)
{
    pnand_info->ch_nr           = 8;
    pnand_info->ce_nr           = 4;
    pnand_info->block_nr        = 512;
    pnand_info->lun_nr          = 2;
    pnand_info->plane_nr        = 2;
    pnand_info->page_nr         = 1024;
    pnand_info->au_nr           = 4;
    pnand_info->cell_bits_nr    = 3;
    assert(pnand_info->ch_nr    < MAX_CHANNEL_NR);
    assert(pnand_info->ce_nr    < MAX_CE_PER_CHANNEL);
    assert(pnand_info->block_nr < MAX_BLOCK_PER_PLANE);
    assert(pnand_info->lun_nr   < MAX_LUN_PER_CE);
    assert(pnand_info->plane_nr < MAX_PLANE_PER_LUN);
    assert(pnand_info->page_nr  < MAX_PAGE_PER_BLOCK);
    printf("current NAND layout :ch_nr %d ce_nr %d lun_nr %d block_nr %d page_nr %d au_nr %d\n",
            pnand_info->ch_nr, pnand_info->ce_nr, pnand_info->lun_nr, pnand_info->block_nr, pnand_info->page_nr, pnand_info->au_nr);

    return true;
}
uint32 l2p_lun_table_initialization(void)
{
    nand_info_t *pnand_info = &gnand_info;
    uint32 llun_nr          = pnand_info->ch_nr * pnand_info->ce_nr * pnand_info->lun_nr;
    memset(l2p_lun_table, 0, sizeof (nand_vector_t) * (MAX_CHANNEL_NR + MAX_CE_PER_CHANNEL + MAX_LUN_PER_CE));
    uint32 ch_cnt = 0, ce_cnt = 0, lun_cnt = 0;

    for (uint32 i = 0; i < llun_nr; i++) {
        l2p_lun_table[i].info.field.plun.field.ch   = ch_cnt;
        l2p_lun_table[i].info.field.plun.field.ce   = ce_cnt;
        l2p_lun_table[i].info.field.plun.field.lun  = lun_cnt;

        if (ch_cnt == pnand_info->ch_nr - 1) {
            ch_cnt = 0;
            if (ce_cnt == pnand_info->ce_nr - 1) {
                ce_cnt = 0;
                if (lun_cnt == pnand_info->lun_nr - 1) {
                    lun_cnt = 0;
                }
                else {
                    lun_cnt++;
                }
            }
            else {
                ce_cnt++;
            }
        }
        else {
            ch_cnt++;
        }
        printf("llun%d => ch %2x ce %2x lun %2x\n",
                i, l2p_lun_table[i].info.field.plun.field.ch,
                l2p_lun_table[i].info.field.plun.field.ce,
                l2p_lun_table[i].info.field.plun.field.lun);
    }
    return true;
}

uint32 lun_paa_list_to_nand_vectors(nand_operator_t *pnand_operator, uint32 lun_paa_cnt)
{
    uint32 i                = 0;
    nand_info_t *pnand_info = &gnand_info;
    uint32 au_nr_per_page   = pnand_info->au_nr;
    nand_lun_paa_t *paa = pnand_operator->lun_paa_operator.list;
    uint32 au_nr_per_plane  = (paa[i].field.pb_type == SLC_OTF_TYPE) ? au_nr_per_page : (au_nr_per_page * pnand_info->cell_bits_nr);
    assert ((au_nr_per_plane & (au_nr_per_plane - 1)) == 0);
    nand_vector_t *cur = NULL;
    if (pnand_operator->vector_operator.list_start)
    {
        cur = pnand_operator->vector_operator.list_start;

    }
    else {
        cur = &pnand_operator->vector_operator.start;
    }
    do {

        cur->info.value         = l2p_lun_table[paa[i].field.llun].info.value;
        cur->info.field.block   = paa->field.psb;
        cur->info.field.plane   = paa[i].field.au_of_lun & (au_nr_per_plane - 1);
        cur->info.field.au_off  = paa[i].field.au_of_lun & (au_nr_per_page - 1);
        cur->nand_operator      = pnand_operator;
        assert(cur);
        i++;
        printf("paa: psb %d au_of_lun %d nand_vector: ch %d ce %d lun %d plane %d block %d au_off %d\n",
                paa->field.psb, paa[i].field.au_of_lun,
                cur->info.field.plun.field.ch,
                cur->info.field.plun.field.ce,
                cur->info.field.plun.field.lun,
                cur->info.field.plane,
                cur->info.field.block,
                cur->info.field.au_off);
        cur = (nand_vector_t *)cur->next;

    } while(i < lun_paa_cnt);

    return true;
}

uint32 lun_paa_range_to_nand_vectors(nand_operator_t *pnand_operator, uint32 lun_paa_cnt)
{
    uint32 i                    = 0;
    nand_info_t *pnand_info     = &gnand_info;
    uint32 au_nr_per_page       = pnand_info->au_nr;
    nand_lun_paa_t *paa_start   = &pnand_operator->lun_paa_operator.start;
    uint32 au_nr_per_plane      = (paa_start[0].field.pb_type == SLC_OTF_TYPE) ? au_nr_per_page : (au_nr_per_page * pnand_info->cell_bits_nr);
    uint32 au_nr_per_plane_bits = __builtin_ctz(au_nr_per_plane);
    nand_vector_t *cur = NULL;
    if (pnand_operator->vector_operator.list_start)
    {
        cur = pnand_operator->vector_operator.list_start;
    }
    else {
        cur = &pnand_operator->vector_operator.start;
    }
    assert ((au_nr_per_plane & (au_nr_per_plane - 1)) == 0);
    do {
        assert(cur);
        cur->info.value         = l2p_lun_table[paa_start[0].field.llun].info.value;
        cur->info.field.block   = paa_start[0].field.psb;
        cur->info.field.plane   = paa_start[0].field.au_of_lun >> au_nr_per_plane_bits;
        cur->info.field.au_off  = paa_start[0].field.au_of_lun & (au_nr_per_page - 1);
        cur->nand_operator      = pnand_operator;
        printf("paa: psb %d au_of_lun %d nand_vector: ch %d ce %d lun %d plane %d block %d au_off %d\n",
                paa_start[0].field.psb, paa_start[0].field.au_of_lun,
                cur->info.field.plun.field.ch,
                cur->info.field.plun.field.ce,
                cur->info.field.plun.field.lun,
                cur->info.field.plane,
                cur->info.field.block,
                cur->info.field.au_off);
        cur = (nand_vector_t *)cur->next;
        paa_start[0].field.au_of_lun++;
        i++;

    } while(i < lun_paa_cnt);

    return true;
}

void nand_vector_pool_initialization(void)
{
    nand_vector_t *nand_vector_pool = (nand_vector_t *)malloc(NAND_VECTOR_POOL_NR * sizeof(nand_vector_t));
    pool_mgr("NANDVECTOR", &gnand_mgr.vector_pool_mgr, sizeof (nand_vector_t), nand_vector_pool, 16);//NAND_VECTOR_POOL_NR
}

void nand_operator_pool_initialization(void)
{
    nand_operator_t *nand_operator_pool = (nand_operator_t *)malloc(NAND_OPERATOR_POOL_NR * sizeof(nand_operator_t));
    pool_mgr("NANDOPERATOR", &gnand_mgr.operator_pool_mgr, sizeof (nand_operator_t), nand_operator_pool, 8);//NAND_OPERATOR_POOL_NR
}

nand_operator_t *nand_allcoate_operator(uint32 want_nr, uint32 *result_nr)
{
    nand_operator_t   *pnand_operator;
    pnand_operator = (nand_operator_t*)pool_allocate_nodes(&gnand_mgr.vector_pool_mgr, result_nr, want_nr);
    return pnand_operator;
}

nand_vector_t* nand_allcoate_vector(uint32 want_nr, uint32 *result_nr)
{
    nand_vector_t* pnand_vector = NULL;
    pnand_vector = (nand_vector_t*)pool_allocate_nodes(&gnand_mgr.vector_pool_mgr, result_nr, want_nr);

    return pnand_vector;
}

uint32 nand_release_vectors(nand_vector_t* start, uint32 vector_cnt)
{
    assert(vector_cnt);
    nand_vector_t *end = start + vector_cnt - 1;
    pool_release_nodes(&gnand_mgr.vector_pool_mgr, (pool_node_t *)start, (pool_node_t *)end);
    return true;
}

void nand_manager_submit(nand_operator_t *nand_operator)
{

}

void nand_operator_submit(mongoc_gridfs_t *gridfs, nand_operator_t *pnand_operator)
{
    uint32 lun_laa_cnt  = pnand_operator->lun_paa_operator.cnt;
    uint32 need_vector_nrs  = 0;
    uint32 got_vectors_nr   = 0;
    //how many nand_vector need to be calculated in here.
    //TODO
    need_vector_nrs = lun_laa_cnt;
    //then to allocate vectors.
    nand_vector_t* pnand_vector = nand_allcoate_vector(lun_laa_cnt, &got_vectors_nr);

    if (need_vector_nrs == got_vectors_nr) {
        pnand_operator->outstanding_vector_cnt      = got_vectors_nr;
        pnand_operator->vector_operator.cnt         = got_vectors_nr;
        pnand_operator->vector_operator.list_start  = pnand_vector;
        lun_paa_range_to_nand_vectors(pnand_operator, lun_laa_cnt);
    }
    else {
        assert(0);
    }
    if (pnand_operator->op_type == OP_TYPE_READ_NORMAL) {
        read_nand_vectors(gridfs, pnand_operator);
    }
    else if (pnand_operator->op_type == OP_TYPE_PROGRAM_NORMAL) {
        write_nand_vectors(gridfs, pnand_operator);
    }
}

void NAND_initialization(void)
{
    nand_info_initialization(&gnand_info);
    nand_vector_pool_initialization();
    nand_operator_pool_initialization();
    l2p_lun_table_initialization();
}

uint32 write_nand_vectors(mongoc_gridfs_t *gridfs, nand_operator_t *nand_operator)
{
    nand_vector_t *cur;
    uint32 vector_cnt = nand_operator->vector_operator.cnt;
    monogodb_operator_t mongodb_operator;

   mongodb_operator.chunk_size    = AU_SIZE;
   mongodb_operator.buf           = nand_operator->buf;
   mongodb_operator.gridfs        = gridfs;


    if (nand_operator->vector_operator.list_start) {
        cur = nand_operator->vector_operator.list_start;
    }
    else {
        cur = &nand_operator->vector_operator.start;
        assert(vector_cnt == 1);
    }
    printf("write vectors\n");
    for (uint32 i = 0; i < vector_cnt; i++) {
        mongodb_operator.name          = cur->info.value;
        mongodb_operator.buf_offset    = i;
        mongodb_write_au(&mongodb_operator);
        cur = (nand_vector_t *)cur->next;
    }

    return true;
}

uint32 read_nand_vectors(mongoc_gridfs_t *gridfs, nand_operator_t *nand_operator)
{
    nand_vector_t *cur;
    uint32 vector_cnt               = nand_operator->vector_operator.cnt;
    monogodb_operator_t mongodb_operator;

    mongodb_operator.chunk_size    = AU_SIZE;
    mongodb_operator.buf           = nand_operator->buf;
    mongodb_operator.gridfs        = gridfs;

    if (nand_operator->vector_operator.list_start) {
        cur = nand_operator->vector_operator.list_start;
    }
    else {
        cur = &nand_operator->vector_operator.start;
    }
    printf("read vectors\n");
    for (uint32 i = 0; i < vector_cnt; i++) {
        mongodb_operator.name          = cur->info.value;
        mongodb_operator.buf_offset    = i;
        mongodb_read_au(&mongodb_operator);
        cur = (nand_vector_t *)cur->next;
    }

    return true;
}

