#include "../lib_header.h"
#include "../nand_vectors/nand_vectors.h"
#include "../simulator/mongodb_operator.h"

uint8 buf[AU_SIZE];
void mongodb_tester(mongoc_gridfs_t *gridfs)
{
    monogodb_operator_t mongodb_operator;

    mongodb_operator.chunk_size    = AU_SIZE;
    mongodb_operator.buf           = buf;
    mongodb_operator.gridfs        = gridfs;
    mongodb_operator.name          = 0x4445678976544;
    mongodb_operator.buf_offset    = 0;
    mongodb_write_au(&mongodb_operator);
    mongodb_read_au(&mongodb_operator);
    mongodb_read_au(&mongodb_operator);
    mongodb_read_au(&mongodb_operator);

}
