#pragma once

typedef struct _mongodb_operator_t
{
    mongoc_gridfs_t *gridfs;
    uint64          name;
    uint16          chunk_size;
    uint16          buf_offset;
    uint8           *buf;
}monogodb_operator_t;
void mongodb_write_au(monogodb_operator_t *poperator);
void mongodb_read_au(monogodb_operator_t *poperator);
