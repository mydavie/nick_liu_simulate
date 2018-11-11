#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <bson.h>
#include <stdio.h>
#include <locale.h>
#include <memory.h>
#include <assert.h>
#include "../utility/types.h"
#include <mongoc/mongoc.h>
#include "../simulator/mongodb_operator.h"
#include "../utility/utility.h"

void mongodb_write_au(monogodb_operator_t *poperator)
{
    bson_string_t *bson_string;
    mongoc_iovec_t iov;
    mongoc_gridfs_file_t *file;
    mongoc_gridfs_file_opt_t opt = {0};
    ssize_t r;

    bson_string = bson_string_new (NULL);
    bson_string_append_printf (bson_string, "%lx", poperator->name);
    opt.filename = bson_string->str;
    opt.chunk_size = poperator->chunk_size;
    file = mongoc_gridfs_create_file (poperator->gridfs, &opt);
    assert(file);
    iov.iov_base    = (void *) (poperator->buf + poperator->buf_offset * poperator->chunk_size);
    iov.iov_len     = poperator->chunk_size;
    memset(iov.iov_base, poperator->buf_offset, iov.iov_len);
    memory_dump_dword("write from buffer : ", poperator->buf_offset, iov.iov_base, iov.iov_len >> 2);
    r = mongoc_gridfs_file_writev (file, &iov, 1, -1);
    bson_string_free (bson_string, true);
    mongoc_gridfs_file_save(file);
    mongoc_gridfs_file_destroy (file);
}

void mongodb_read_au(monogodb_operator_t *poperator)
{
    bson_string_t *bson_string;
    mongoc_iovec_t iov;
    mongoc_gridfs_file_t *file;
    mongoc_gridfs_file_opt_t opt = {0};
    ssize_t r;
    mongoc_stream_t *stream;
    bson_error_t error;

    bson_string = bson_string_new (NULL);
    bson_string_append_printf (bson_string, "%lx", poperator->name);
    opt.filename = bson_string->str;
    opt.chunk_size = poperator->chunk_size;
    file = mongoc_gridfs_create_file (poperator->gridfs, &opt);
    assert(file);
    iov.iov_base    = (void *) (poperator->buf + poperator->buf_offset * poperator->chunk_size);
    iov.iov_len     = poperator->chunk_size;

    memset(iov.iov_base, 0x55, iov.iov_len);
    memory_dump_dword("reset buffer : ", poperator->buf_offset, iov.iov_base, iov.iov_len >> 2);
    file = mongoc_gridfs_find_one_by_filename (poperator->gridfs, opt.filename, &error);
    stream = mongoc_stream_gridfs_new (file);
    assert (stream);

    for (;;) {
        r = mongoc_stream_readv (stream, &iov, 1, -1, 0);

        assert (r >= 0);
        if (r == 0) {
           break;
        }
        else {
            memory_dump_dword("read to buffer : ", poperator->buf_offset, iov.iov_base, iov.iov_len >> 2);
        }
    }
    mongoc_stream_destroy (stream);
    bson_string_free (bson_string, true);
    mongoc_gridfs_file_save(file);
    mongoc_gridfs_file_destroy (file);
}
