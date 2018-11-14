#include <memory.h>
#include "../nand_vectors/nand_vectors.h"
#include "../utility/types.h"
#include "../utility/utility.h"
#include "logical_lun_tester.h"
#include "../nand_vectors/nand_vectors.h"

void nand_vector_tester(mongoc_gridfs_t *gridfs)
{
    nand_vector_t nand_vector;
    uint8 *buf;
    buf = (uint8 *)align_memory_malloc(AU_SIZE, 64);
    memset(buf, 0x22, AU_SIZE);
    nand_vector.au_cnt = 4;
    nand_vector.info.value = 0;
    nand_vector.simulator_ptr = gridfs;
    write_nand_vector(&nand_vector, buf);
    nand_vector.au_cnt = 4;
    nand_vector.info.value = 0;
    nand_vector.simulator_ptr = gridfs;
    read_nand_vector(&nand_vector, buf);
    align_memory_free(buf);
}
