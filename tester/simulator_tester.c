#include <memory.h>
#include "../nand_vectors/nand_vectors.h"
#include "../utility/types.h"
#include "../utility/utility.h"
#include "logical_lun_tester.h"
#include "../simulator/mongodb_operator.h"


void mongodb_tester(mongoc_gridfs_t *gridfs)
{
    monogodb_operator_t mongodb_operator;

    mongodb_operator.chunk_size    = AU_SIZE;
    mongodb_operator.buf           = (uint8 *)align_memory_malloc(AU_SIZE, 64);
    mongodb_operator.gridfs        = gridfs;
    mongodb_operator.name          = 0x4445678976544;
    mongodb_operator.buf_offset    = 0;
    mongodb_write_au(&mongodb_operator);
    mongodb_read_au(&mongodb_operator);
    align_memory_free(mongodb_operator.buf);
}
