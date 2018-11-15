#include "../lib_header.h"
#include "../simulator/mongodb_operator.h"
#include "../nand_vectors/nand_vectors.h"

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
    nand_vector_t *nand_vector_pool = align_memory_malloc(NAND_VECTOR_POOL_NR * sizeof(nand_vector_t), sizeof (uint64));
    pool_mgr("NAND VECTOR", &gnand_mgr.vector_pool_mgr, sizeof (nand_vector_t), nand_vector_pool, 50);//NAND_VECTOR_POOL_NR

}

nand_vector_t* nand_allcoate_vector(uint32 want_nr, uint32 *result_nr)
{
    nand_vector_t* pnand_vector = NULL;
    assert(gnand_mgr.vector_pool_mgr.node_sz = sizeof (nand_vector_t));
    pnand_vector = (nand_vector_t*)pool_allocate_nodes(&gnand_mgr.vector_pool_mgr, result_nr, want_nr);
    memset(((uint8*)pnand_vector + sizeof (pool_node_t)), 0, gnand_mgr.vector_pool_mgr.node_sz - sizeof (pool_node_t));
    return pnand_vector;
}

uint32 nand_release_vectors(nand_vector_t* start, uint32 vector_cnt)
{
    assert(vector_cnt);
    assert(start);
    pool_release_nodes(&gnand_mgr.vector_pool_mgr, (pool_node_t *)start, vector_cnt);
    return true;
}

void nand_init_onetime(void)
{
    nand_info_init_onetime(&gnand_info);
    nand_vector_pool_init_onetime();
}


uint32 write_nand_vector(nand_vector_t* pnand_vector)
{
    monogodb_operator_t mongodb_operator;
    nand_vector_t  cur;
    memory_node_t *pbuf_node        = pnand_vector->buf_node;
    cur.info.value 					= pnand_vector->info.value;
    mongodb_operator.chunk_size		= AU_SIZE;
    mongodb_operator.gridfs			= pnand_vector->simulator_ptr;

    for (uint32 i = 0; i < pnand_vector->au_cnt; i++) {
        mongodb_operator.name          	= cur.info.value;
        mongodb_operator.buf			= pbuf_node->buf;
        mongodb_operator.buf_offset     = 0;
        assert(pbuf_node->buf);
        printf("program nand vector %16lx plun %d plane %d block %d page %d au_off %d buffer %lx\n",
        		cur.info.value,
				cur.info.field.plun.value,
				cur.info.field.plane,
				cur.info.field.block,
				cur.info.field.page,
				cur.info.field.au_off,
				(uint64)(mongodb_operator.buf));
        mongodb_write_au(&mongodb_operator);
        assert(cur.info.field.au_off < AU_NR_PER_PAGE);
        cur.info.field.au_off += 1;
        pbuf_node = (memory_node_t*)pbuf_node->next;
    }
    assert(mongodb_operator.buf_offset < AU_NR_PER_PAGE);

    return true;
}

uint32 read_nand_vector(nand_vector_t* pnand_vector)
{
    monogodb_operator_t mongodb_operator;
    nand_vector_t  cur;
    memory_node_t *pbuf_node        = pnand_vector->buf_node;
	cur.info.value 					= pnand_vector->info.value;
    mongodb_operator.chunk_size		= AU_SIZE;
    mongodb_operator.gridfs			= pnand_vector->simulator_ptr;

    for (uint32 i = 0; i < pnand_vector->au_cnt; i++) {
        mongodb_operator.name          	= cur.info.value;
        mongodb_operator.buf			= pbuf_node->buf;
        mongodb_operator.buf_offset     = 0;
        assert(pbuf_node->buf);
        printf("read nand vector: %16lx >> plun %d plane %d block %d page %d au_off %d buffer %lx\n",
        			cur.info.value,
					cur.info.field.plun.value,
					cur.info.field.plane,
					cur.info.field.block,
					cur.info.field.page,
					cur.info.field.au_off,
      				(uint64)(mongodb_operator.buf));
        mongodb_read_au(&mongodb_operator);
        assert(cur.info.field.au_off < AU_NR_PER_PAGE);
        cur.info.field.au_off += 1;
        pbuf_node = (memory_node_t*)pbuf_node->next;
    }
    assert(mongodb_operator.buf_offset < AU_NR_PER_PAGE);
    return true;
}

