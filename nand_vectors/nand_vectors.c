#include "../nand_vectors/nand_vectors.h"
#include "../nand_vectors/nand_vectors.h"

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

nand_info_t gnand_info;
nand_mgr_t  gnand_mgr;

//the info will get from NAND parameter page.
uint32 nand_info_init_onetime(nand_info_t *pnand_info)
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

nand_info_t * get_nand_info(void)
{
    return &gnand_info;
}

void nand_vector_pool_init_onetime(void)
{
    nand_vector_t *nand_vector_pool = (nand_vector_t *)malloc(NAND_VECTOR_POOL_NR * sizeof(nand_vector_t));
    pool_mgr("NANDVECTOR", &gnand_mgr.vector_pool_mgr, sizeof (nand_vector_t), nand_vector_pool, 16);//NAND_VECTOR_POOL_NR
}

//nand_operator_t *nand_allcoate_operator(uint32 want_nr, uint32 *result_nr)
//{
//    nand_operator_t   *pnand_operator;
//    pnand_operator = (nand_operator_t*)pool_allocate_nodes(&gnand_mgr.vector_pool_mgr, result_nr, want_nr);
//    return pnand_operator;
//}

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

//
//void nand_operator_submit(mongoc_gridfs_t *gridfs, nand_operator_t *pnand_operator)
//{
//    uint32 lun_laa_cnt  = pnand_operator->logical_lun_operator.cnt;
//    uint32 need_vector_nrs  = 0;
//    //how many nand_vector need to be calculated in here.
//    //TODO
//    need_vector_nrs = lun_laa_cnt;
//    //then to allocate vectors.
//    logcial_lun_t *plogical_lun_list = pnand_operator->logical_lun_operator.list;
//    uint32 cnt = pnand_operator->logical_lun_operator.cnt;
//
//    if (plogical_lun_list)
//    {
//        logical_lun_list_to_vectors(plogical_lun_list, cnt);
//    }
//    else {
//        logical_lun_to_vectors(&pnand_operator->logical_lun_operator.start);
//    }
//
//    if (pnand_operator->op_type == OP_TYPE_READ_NORMAL) {
//        //read_nand_vectors(gridfs, pnand_operator);
//    }
//    else if (pnand_operator->op_type == OP_TYPE_PROGRAM_NORMAL) {
//        //write_nand_vectors(gridfs, pnand_operator);
//    }
//}

void nand_init_onetime(void)
{
    nand_info_init_onetime(&gnand_info);
    nand_vector_pool_init_onetime();
}

uint32 write_nand_vector(nand_vector_t* pnand_vector, uint8 *buf)
{
    monogodb_operator_t mongodb_operator;

    mongodb_operator.chunk_size    = AU_SIZE;
    mongodb_operator.buf           = buf;
    mongodb_operator.gridfs        = pnand_vector->simulator_ptr;


    for (uint32 i = 0; i < pnand_vector->au_cnt; i++) {
        mongodb_operator.name          	= pnand_vector->info.value;
        mongodb_operator.buf_offset    	= i;
        printf("program nand vector %16lx plun %d plane %d block %d page %d au_off %d buffer %x\n",
        		pnand_vector->info.value,
        		pnand_vector->info.field.plun.value,
				pnand_vector->info.field.plane,
				pnand_vector->info.field.block,
				pnand_vector->info.field.page,
				pnand_vector->info.field.au_off,
				(uint32)(mongodb_operator.buf + i * AU_SIZE));
        mongodb_write_au(&mongodb_operator);
        pnand_vector->info.field.au_off += 1;
    }

    return true;
}

uint32 read_nand_vector(nand_vector_t* pnand_vector, uint8 *buf)
{
    monogodb_operator_t mongodb_operator;

    mongodb_operator.chunk_size    = AU_SIZE;
    mongodb_operator.buf           = buf;
    mongodb_operator.gridfs        = pnand_vector->simulator_ptr;

    for (uint32 i = 0; i < pnand_vector->au_cnt; i++) {
        mongodb_operator.name          	= pnand_vector->info.value;
        mongodb_operator.buf_offset    	= i;
        printf("read nand vector %16lx plun %d plane %d block %d page %d au_off %d buffer %x\n",
              		pnand_vector->info.value,
              		pnand_vector->info.field.plun.value,
      				pnand_vector->info.field.plane,
      				pnand_vector->info.field.block,
      				pnand_vector->info.field.page,
      				pnand_vector->info.field.au_off,
      				(uint32)(mongodb_operator.buf + i * AU_SIZE));
        mongodb_read_au(&mongodb_operator);
        pnand_vector->info.field.au_off += 1;
    }

    return true;
}

