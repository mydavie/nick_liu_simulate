#include <memory.h>
#include "../nand_vectors/nand_vectors.h"
#include "../utility/types.h"
#include "../utility/utility.h"
#include "logical_lun_tester.h"
#include "../nand_vectors/nand_vectors.h"

void nand_vector_tester(mongoc_gridfs_t *gridfs)
{
    nand_vector_t nand_vector;
    uint8 *buf = (uint8 *)malloc(AU_SIZE);
    memset(buf, 0x22, AU_SIZE);
    nand_vector.au_cnt = 1;
    nand_vector.info.value = 0;
    nand_vector.simulator_ptr = gridfs;
    write_nand_vector(&nand_vector, buf);
    read_nand_vector(&nand_vector, buf);
    free(buf);
}
