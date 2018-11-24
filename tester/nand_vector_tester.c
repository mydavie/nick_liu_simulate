#include "../lib_header.h"
#include "../nand_vectors/nand_vectors.h"

void nand_vector_tester(mongoc_gridfs_t *gridfs)
{
    nand_vector_t nand_vector;
    uint32 result_nr;
    memory_node_t *node = NULL;

    nand_vector.au_cnt                      = 3;
    nand_vector.vector.phy_lun.value        = 0;
    nand_vector.vector.phy_lun_ptr.value    = 0;
    nand_vector.simulator                   = gridfs;
    node = memory_allcoate_nodes(nand_vector.au_cnt, &result_nr);
    assert(result_nr == nand_vector.au_cnt);
    nand_vector.buf_node                    = node;
    write_nand_vector(&nand_vector);
    nand_vector.au_cnt                      = 3;
    nand_vector.vector.phy_lun.value        = 0;
    nand_vector.vector.phy_lun_ptr.value    = 0;
    nand_vector.simulator                   = gridfs;
    nand_vector.buf_node                    = node;
    read_nand_vector(&nand_vector);
    memory_release_nodes(node, result_nr);
}
