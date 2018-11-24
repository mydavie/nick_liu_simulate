#include "../lib_header.h"

#include "../simulator/mongodb_operator.h"


#define MONGO_DB_MK	0xFB
void mongodb_write_au(monogodb_operator_t *poperator)
{
    bson_string_t *bson_string;
    mongoc_iovec_t iov;
    mongoc_gridfs_file_t *file;
    mongoc_gridfs_file_opt_t opt = {0};

    bson_string = bson_string_new (NULL);
    bson_string_append_printf (bson_string, "%lx", poperator->name);
    opt.filename = bson_string->str;
    opt.chunk_size = poperator->chunk_size;
    file = mongoc_gridfs_create_file (poperator->gridfs, &opt);
    assert(file);
    iov.iov_base    = (void *) (poperator->buf + poperator->buf_offset);
    iov.iov_len     = poperator->chunk_size;
    assert((iov.iov_len >> 6) - 1 >= 0);
    assert((((uint64)iov.iov_base) & ((1UL << 6) - 1)) == 0);
    ((uint64*)iov.iov_base)[0] = poperator->name;
    for (uint32 i = 1; i < (iov.iov_len >> 6); i++) {
    	((uint64*)iov.iov_base)[i] = i;
    }
    memory_dump_u64("write from buffer : ", poperator->buf_offset, iov.iov_base, iov.iov_len >> 6);
    mongoc_gridfs_file_writev (file, &iov, 1, -1);
    mongoc_gridfs_file_save(file);
    bson_string_free (bson_string, true);
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
    iov.iov_base    = (void *) (poperator->buf + poperator->buf_offset);
    iov.iov_len     = poperator->chunk_size;
    assert((iov.iov_len >> 6) - 1 >= 0);
    assert((((uint64)iov.iov_base) & ((1UL << 6) - 1)) == 0);
    memset(iov.iov_base, 0x55, iov.iov_len);
    //memory_dump_u64("reset buffer : ", poperator->buf_offset, iov.iov_base, iov.iov_len >> 6);
    file = mongoc_gridfs_find_one_by_filename (poperator->gridfs, opt.filename, &error);

    if (file) {
		stream = mongoc_stream_gridfs_new (file);
		assert (stream);

		for (;;) {
			r = mongoc_stream_readv (stream, &iov, 1, -1, 0);
			assert (r >= 0);
			if (r == 0) {
			   break;
			}
			else {
				memory_dump_u64("read and compare buffer : ", poperator->buf_offset, iov.iov_base, iov.iov_len >> 6);
				if (((uint64*)iov.iov_base)[0] != poperator->name) {
					printf("data compare error\n");
					assert(0);
				}
			}
		}
		mongoc_stream_destroy (stream);
		mongoc_gridfs_file_destroy (file);
    }
    else {
    	printf("not found this data, erased page\n");
    }

    bson_string_free (bson_string, true);

}
