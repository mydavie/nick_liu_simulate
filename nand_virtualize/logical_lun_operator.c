#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <bson.h>
#include <locale.h>
#include <memory.h>
#include <assert.h>
#include <mongoc/mongoc.h>
#include "../utility/types.h"
#include "../utility/utility.h"
#include "../utility/pool_mgr.h"
#include "../simulator/mongodb_operator.h"
#include "../nand_vectors/nand_vectors.h"
#include "logical_lun_operator.h"

physical_lun_t l2p_lun_table[MAX_LLUN_NR];

uint32 logical_lun_to_physical_lun(uint32 logical_lun_offset)
{
    return l2p_lun_table[logical_lun_offset].value;
}

uint32 l2p_lun_table_init_onetime(void)
{
    nand_info_t *pnand_info = get_nand_info();
    uint32 llun_nr          = pnand_info->ch_nr * pnand_info->ce_nr * pnand_info->lun_nr;
    assert(llun_nr < MAX_LLUN_NR);

    memset(l2p_lun_table, 0, sizeof (physical_lun_t) * MAX_LLUN_NR);
    uint32 ch_cnt = 0, ce_cnt = 0, lun_cnt = 0;

    for (uint32 i = 0; i < llun_nr; i++) {
        l2p_lun_table[i].field.ch   = ch_cnt;
        l2p_lun_table[i].field.ce   = ce_cnt;
        l2p_lun_table[i].field.lun  = lun_cnt;

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
        printf("logical lun %d =>physical lun: [ ch %2x ce %2x lun %2x]\n",
                i, l2p_lun_table[i].field.ch,
                l2p_lun_table[i].field.ce,
                l2p_lun_table[i].field.lun);
    }
    return true;
}

uint32 fill_nand_vectors(logical_lun_t *plogical_lun, nand_vector_t* pnand_vector)
{
    uint32 i                        = 0;
    nand_info_t *pnand_info         = get_nand_info();
    uint32 au_nr_per_page           = pnand_info->au_nr;
    uint32 au_nr_per_plane          = (plogical_lun->llun_nand_type == SLC_OTF_TYPE) ? au_nr_per_page : (au_nr_per_page * pnand_info->cell_bits_nr);
    uint32 au_nr_per_plane_bits     = __builtin_ctz(au_nr_per_plane);
    nand_vector_t *cur              = pnand_vector;
    uint32 au_allocated_ptr         = plogical_lun->au_param.range.au_start;
    uint32 au_allocated_cnt         = plogical_lun->au_param.range.au_cnt;
    uint32 au_of_start              = au_allocated_ptr & (AU_NR_PER_PAGE - 1);
    uint32 au_front_cnt             = 0;
    uint32 vector_cnt               = 0;
    uint32 au_back_cnt              = 0;
    //to split one au range of current logical lun to different plane based(this will modified based on the ASIC design)
    if (au_of_start) {
        cur->logcial_lun = plogical_lun;
        au_front_cnt = (au_allocated_cnt <= AU_NR_PER_PAGE - au_of_start) ? au_allocated_cnt : (AU_NR_PER_PAGE - au_of_start);
        assert(cur);
        cur->info.value         = logical_lun_to_physical_lun(plogical_lun->llun_offset);
        cur->info.field.block   = plogical_lun->llun_spb_id;
        cur->info.field.plane   = au_allocated_ptr >> au_nr_per_plane_bits;
        cur->info.field.au_off  = au_allocated_ptr & (au_nr_per_page - 1);
        cur->au_cnt             = au_front_cnt;
        au_allocated_cnt        -= au_front_cnt;
        au_allocated_ptr        += au_front_cnt;
        cur = (nand_vector_t *)cur->next;
        vector_cnt++;
      }
    while (au_allocated_cnt && (au_allocated_cnt > AU_NR_PER_PAGE)) {

        cur->logcial_lun = plogical_lun;
        assert(cur);
        cur->info.value         = logical_lun_to_physical_lun(plogical_lun->llun_offset);
        cur->info.field.block   = plogical_lun->llun_spb_id;
        cur->info.field.plane   = au_allocated_ptr >> au_nr_per_plane_bits;
        cur->info.field.au_off  = au_allocated_ptr & (au_nr_per_page - 1);
        cur->au_cnt             = AU_NR_PER_PAGE;
        au_allocated_cnt        -= AU_NR_PER_PAGE;
        au_allocated_ptr        += AU_NR_PER_PAGE;
        vector_cnt++;
        cur = (nand_vector_t *)cur->next;
    }
    if (au_allocated_cnt) {
        cur->logcial_lun = plogical_lun;
        assert(cur);
        cur->info.value         = logical_lun_to_physical_lun(plogical_lun->llun_offset);
        cur->info.field.block   = plogical_lun->llun_spb_id;
        cur->info.field.plane   = au_allocated_ptr >> au_nr_per_plane_bits;
        cur->info.field.au_off  = 0;
        cur->au_cnt             = au_allocated_cnt;
        au_allocated_ptr        += au_allocated_cnt;
        au_allocated_cnt        = 0;
        au_back_cnt++;
        vector_cnt++;
    }
    assert(au_allocated_cnt == 0);
    assert(au_allocated_ptr == plogical_lun->au_param.range.au_start + plogical_lun->au_param.range.au_cnt);
    //dump info
    cur = pnand_vector;
    do {
        assert(cur);
        printf("vector[%d]: psb %d au_of_lun %d nand_vector: ch %d ce %d lun %d plane %d block %d au_off %d cnt %d\n",
                i, plogical_lun->llun_spb_id, plogical_lun->au_param.range.au_start ,
                cur->info.field.plun.field.ch,
                cur->info.field.plun.field.ce,
                cur->info.field.plun.field.lun,
                cur->info.field.plane,
                cur->info.field.block,
                cur->info.field.au_off,
				cur->au_cnt);
        cur = (nand_vector_t *)cur->next;
        i++;
    } while(i < vector_cnt);

    return vector_cnt;
}

void logical_lun_to_vectors(logical_lun_t *plogical_lun)
{
    uint32 au_cnt                   = plogical_lun->au_param.range.au_cnt;
    uint32 au_start                 = plogical_lun->au_param.range.au_start;
    uint32 vector_cnt               = 0;
    uint32 au_of_start              = au_start & (AU_NR_PER_PAGE - 1);
    uint32 au_front_cnt             = 0;
    uint32 au_middle_cnt            = 0;
    uint32 au_back_cnt              = 0;
    uint32 got_vectors_nr           = 0;
    nand_vector_t* pnand_vector     = NULL;

    if (au_of_start) {
        vector_cnt++;
        au_front_cnt = (au_cnt <= AU_NR_PER_PAGE - au_of_start) ? au_cnt : (AU_NR_PER_PAGE - au_of_start);
        au_cnt -= au_front_cnt;
    }
    while (au_cnt && au_cnt > AU_NR_PER_PAGE) {
        au_cnt -= AU_NR_PER_PAGE;
        vector_cnt++;
        au_middle_cnt++;
    }
    if (au_cnt) {
        vector_cnt++;
        au_back_cnt++;
    }
    pnand_vector = nand_allcoate_vector(vector_cnt, &got_vectors_nr);
    if (vector_cnt != got_vectors_nr) {
        nand_release_vectors(pnand_vector, got_vectors_nr);
    }
    else {
        plogical_lun->nand_vector = pnand_vector;
        plogical_lun->vector_cnt  = vector_cnt;
        assert(got_vectors_nr == fill_nand_vectors(plogical_lun, pnand_vector));
    }
}

void logical_lun_list_to_vectors(logical_lun_t *plogical_lun, uint32 logical_lun_list_cnt)
{
    logical_lun_t *cur = plogical_lun;
    uint32 i = 0;

    do {
        logical_lun_to_vectors(cur);
        i++;
    } while (i < logical_lun_list_cnt);
}

uint32 write_logical_lun(logical_lun_t *plogical_lun)
{
    nand_vector_t *cur      = plogical_lun->nand_vector;
    uint32 vector_cnt       = plogical_lun->vector_cnt;
    mongoc_gridfs_t *gridfs = plogical_lun->simulator_ptr;
    uint8* buf              = plogical_lun->buf;

    for (uint32 i = 0; i < vector_cnt; i++) {
        cur->simulator_ptr = gridfs;
        write_nand_vector(cur, buf);
        cur = (nand_vector_t *)cur->next;
        buf += cur->au_cnt * AU_SIZE;
    }
    return true;
}

uint32 read_logical_lun(logical_lun_t *plogical_lun)
{
    nand_vector_t *cur      = plogical_lun->nand_vector;
    uint32 vector_cnt       = plogical_lun->vector_cnt;
    mongoc_gridfs_t *gridfs = plogical_lun->simulator_ptr;
    uint8* buf              = plogical_lun->buf;

    for (uint32 i = 0; i < vector_cnt; i++) {
        cur->simulator_ptr = gridfs;
        read_nand_vector(cur, buf);
        cur = (nand_vector_t *)cur->next;
        buf += cur->au_cnt * AU_SIZE;
    }
    return true;
}

uint32 submit_logical_lun_operator(logcial_lun_operator_t *plogcial_lun_operator)
{
    uint32 logical_lun_cnt              = plogcial_lun_operator->cnt;
    logical_lun_t *cur                  = plogcial_lun_operator->list;
    mongoc_gridfs_t *gridfs             = plogcial_lun_operator->simulator_ptr;

    if (cur)
    {
        logical_lun_list_to_vectors(cur, logical_lun_cnt);
    }
    else {
        logical_lun_to_vectors(&plogcial_lun_operator->start);
    }

    if (cur == NULL) {
        cur = &plogcial_lun_operator->start;
        assert(logical_lun_cnt == 1);
    }
    for (uint32 i = 0; i < logical_lun_cnt; i++) {
        cur->simulator_ptr = gridfs;
        if (plogcial_lun_operator->op_type == LOGICAL_LUN_OP_TYPE_PROGRAM) {
            write_logical_lun(cur);
        }
        else {
            read_logical_lun(cur);
        }
    }
    return true;
}

pool_mgr_t logical_lun_pool_mgr;
void logical_lun_pool_init_onetime(void)
{
    logical_lun_t *logical_lun_pool = (logical_lun_t *)malloc(MAX_LLUN_NR * sizeof(logical_lun_t));
    pool_mgr("LOGICALLUN", &logical_lun_pool_mgr, sizeof (logical_lun_t), logical_lun_pool, MAX_LLUN_NR);
}

logical_lun_t* logical_lun_allcoate(uint32 want_nr, uint32 *result_nr)
{
    logical_lun_t* plogical_lun = NULL;
    assert(logical_lun_pool_mgr.node_sz = sizeof (logical_lun_t));
    plogical_lun = (logical_lun_t*)pool_allocate_nodes(&logical_lun_pool_mgr, result_nr, want_nr);

    return plogical_lun;
}

uint32 logical_lun_release(logical_lun_t* start, uint32 vector_cnt)
{
    assert(vector_cnt);
    logical_lun_t *end = start + vector_cnt - 1;
    assert(logical_lun_pool_mgr.node_sz = sizeof (logical_lun_t));
    pool_release_nodes(&logical_lun_pool_mgr, (pool_node_t *)start, (pool_node_t *)end);
    return true;
}

void logical_lun_init_onetime(void)
{
    logical_lun_pool_init_onetime();
    l2p_lun_table_init_onetime();
}





