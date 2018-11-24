#include "../lib_header.h"
#include "../simulator/mongodb_operator.h"
#include "../nand_vectors/nand_vectors.h"

nand_info_t gnand_info;
nand_mgr_t  gnand_mgr;

//the info will get from NAND parameter page.
uint32 nand_info_init_onetime(nand_info_t *pnand_info)
{
    pnand_info->ch_nr                   = 8;
    pnand_info->ce_nr                   = 4;
    pnand_info->block_nr_per_plane      = 512;
    pnand_info->lun_nr                  = 2;
    pnand_info->plane_nr_per_lun        = 2;
    pnand_info->page_nr_per_block       = 1024;
    pnand_info->au_nr_per_page_width    = 4;
    pnand_info->bits_nr_per_cell        = 3;
    assert(pnand_info->ch_nr <= MAX_CHANNEL_NR);
    assert(pnand_info->ce_nr <= MAX_CE_PER_CHANNEL);
    assert(pnand_info->block_nr_per_plane <= MAX_BLOCK_PER_PLANE);
    assert(pnand_info->lun_nr <= MAX_LUN_PER_CE);
    assert(pnand_info->plane_nr_per_lun <= MAX_PLANE_PER_LUN);
    assert(pnand_info->page_nr_per_block  <= MAX_PAGE_PER_BLOCK);
    pnand_info->au_nr_per_slc_plane         = pnand_info->page_nr_per_block * pnand_info->au_nr_per_page_width;
    pnand_info->au_nr_per_slc_lun           = pnand_info->au_nr_per_slc_plane * pnand_info->plane_nr_per_lun;
    pnand_info->au_nr_per_slc_plane_width   = pnand_info->au_nr_per_page_width;
    pnand_info->au_nr_per_slc_lun_width     = pnand_info->au_nr_per_page_width * pnand_info->plane_nr_per_lun;
    pnand_info->au_nr_per_xlc_plane         = pnand_info->page_nr_per_block * pnand_info->au_nr_per_page_width;
    pnand_info->au_nr_per_xlc_lun           = pnand_info->au_nr_per_slc_plane * pnand_info->plane_nr_per_lun;
    pnand_info->au_nr_per_xlc_plane_width   = pnand_info->au_nr_per_page_width;
    pnand_info->au_nr_per_xlc_lun_width     = pnand_info->au_nr_per_page_width * pnand_info->plane_nr_per_lun;
//    printf("MAX NAND layout : ch_nr %d ce_nr %d lun_nr %d block_nr %d page_nr %d au_nr %d\n",
//    		MAX_CHANNEL_NR, MAX_CE_PER_CHANNEL, MAX_LUN_PER_CE, MAX_BLOCK_PER_PLANE, MAX_PAGE_PER_BLOCK, AU_NR_PER_PAGE);
    printf("current NAND layout :ch_nr %d ce_nr %d lun_nr %d block_nr %d page_nr %d au_nr %d\n",
            pnand_info->ch_nr, pnand_info->ce_nr, pnand_info->lun_nr,
            pnand_info->block_nr_per_plane, pnand_info->page_nr_per_block, pnand_info->au_nr_per_page_width);
//    printf("paralel au nr %d max logical lun nr %d au per lun %d\n",MAX_AU_PER_PARALEL, MAX_LLUN_NR, MAX_AU_PER_LUN);
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
    assert(gnand_mgr.vector_pool_mgr.node_sz == sizeof (nand_vector_t));
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
    assert(pnand_vector->simulator);
    cur.vector.phy_lun.value        = pnand_vector->vector.phy_lun.value;
    mongodb_operator.chunk_size     = AU_SIZE;
    mongodb_operator.gridfs         = pnand_vector->simulator;

    for (uint32 i = 0; i < pnand_vector->au_cnt; i++) {
        mongodb_operator.name           = ((uint64)cur.vector.phy_lun.value) << 32 | cur.vector.phy_lun_ptr.value;
        mongodb_operator.buf            = pbuf_node->buf;
        mongodb_operator.buf_offset     = 0;
        assert(pbuf_node->buf);
        printf("program nand vector %16lx plun %d plane %d block %d page %d au_off %d buffer %lx\n",
                mongodb_operator.name,
                pnand_vector->vector.phy_lun.value,
                pnand_vector->vector.phy_lun_ptr.field.plane,
                pnand_vector->vector.phy_lun_ptr.field.block,
                pnand_vector->vector.phy_lun_ptr.field.page,
                pnand_vector->vector.phy_lun_ptr.field.au_off,
                (uint64)(mongodb_operator.buf));
        mongodb_write_au(&mongodb_operator);
        assert(pnand_vector->vector.phy_lun_ptr.field.au_off < AU_NR_PER_PAGE);
        pnand_vector->vector.phy_lun_ptr.field.au_off += 1;
        pbuf_node = (memory_node_t*)pbuf_node->next;
    }
    assert(mongodb_operator.buf_offset < AU_NR_PER_PAGE);

    return true;
}

uint32 read_nand_vector(nand_vector_t* pnand_vector)
{
    monogodb_operator_t mongodb_operator;
    memory_node_t *pbuf_node        = pnand_vector->buf_node;
    assert(pnand_vector->simulator);
    mongodb_operator.chunk_size     = AU_SIZE;
    mongodb_operator.gridfs         = pnand_vector->simulator;

    for (uint32 i = 0; i < pnand_vector->au_cnt; i++) {
        mongodb_operator.name       = ((uint64)pnand_vector->vector.phy_lun.value) << 32 | pnand_vector->vector.phy_lun_ptr.value;
        mongodb_operator.buf        = pbuf_node->buf;
        mongodb_operator.buf_offset = 0;
        assert(pbuf_node->buf);
        printf("read nand vector: %16lx >> plun %d plane %d block %d page %d au_off %d buffer %lx\n",
                    mongodb_operator.name,
                    pnand_vector->vector.phy_lun.value,
                    pnand_vector->vector.phy_lun_ptr.field.plane,
                    pnand_vector->vector.phy_lun_ptr.field.block,
                    pnand_vector->vector.phy_lun_ptr.field.page,
                    pnand_vector->vector.phy_lun_ptr.field.au_off,
                    (uint64)(mongodb_operator.buf));
        mongodb_read_au(&mongodb_operator);
        assert(pnand_vector->vector.phy_lun_ptr.field.au_off < AU_NR_PER_PAGE);
        pnand_vector->vector.phy_lun_ptr.field.au_off += 1;
        pbuf_node = (memory_node_t*)pbuf_node->next;
    }
    assert(mongodb_operator.buf_offset < AU_NR_PER_PAGE);
    return true;
}

