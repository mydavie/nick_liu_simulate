#include <assert.h>
#include <mongoc/mongoc.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <bson.h>
#include <stdio.h>
#include <locale.h>
#include <memory.h>

typedef unsigned int    uint32;
typedef unsigned long   uint64;
typedef unsigned short  uint16;

#define KB_BITS                     10
#define KB                          (1 << KB_BITS)

//NAND layout bits limitation
#define MAX_AU_BITS                 (2 + KB_BITS)
#define MAX_PAGE_SIZE_BITS          (4 + KB_BITS)
#define MAX_AU_PER_PAGE_BITS        (MAX_PAGE_SIZE_BITS - MAX_AU_BITS)
#define MAX_CH_BITS                 4
#define MAX_CE_PER_CH_BITS          4
#define MAX_LUN_PER_CE_BITS         2
#define MAX_PLANE_PER_LUN_BITS      2
#define MAX_BLOCK_PER_PLANE_BITS    10
#define MAX_PAGE_PER_BLOCK_BITS     14

#define MAX_NAND_LAYOUT_BITS          (MAX_AU_PER_PAGE_BITS + MAX_CH_BITS + MAX_CE_PER_CH_BITS + \
    MAX_LUN_PER_CE_BITS + MAX_PLANE_PER_LUN_BITS + MAX_BLOCK_PER_PLANE_BITS + MAX_PAGE_PER_BLOCK_BITS)

#define PAGE_SIZE                   (1UL << MAX_PAGE_SIZE_BITS)
#define AU_SIZE                     (1UL << MAX_AU_BITS)
#define AU_NR_PER_PAGE              (1UL << MAX_AU_PER_PAGE_BITS)
#define MAX_CHANNEL_NR              (1UL << MAX_CH_BITS)
#define MAX_CE_PER_CHANNEL          (1UL << MAX_CE_PER_CH_BITS)
#define MAX_LUN_PER_CE              (1UL << MAX_LUN_PER_CE_BITS)
#define MAX_PLANE_PER_LUN           (1UL << MAX_PLANE_PER_LUN_BITS)
#define MAX_BLOCK_PER_PLANE         (1UL << MAX_BLOCK_PER_PLANE_BITS)
#define MAX_PAGE_PER_BLOCK          (1UL << MAX_PAGE_PER_BLOCK_BITS)
#define MAX_AU_PER_SUPERPB          MAX_NAND_LAYOUT_BITS

typedef union _nand_vector_t
{
    uint64 value;
    struct
    {
        uint64 ch       : MAX_CH_BITS;
        uint64 ce       : MAX_CE_PER_CH_BITS;
        uint64 lun		: MAX_LUN_PER_CE_BITS;
        uint64 plane	: MAX_PLANE_PER_LUN_BITS;
        uint64 block	: MAX_BLOCK_PER_PLANE_BITS;
        uint64 page 	: MAX_PAGE_PER_BLOCK_BITS;
        uint64 au_off	: MAX_AU_PER_PAGE_BITS;
        uint64 au_nr	: MAX_AU_PER_PAGE_BITS + 1;
        uint64 rsvd     : 64 - MAX_NAND_LAYOUT_BITS - 1;
	} field;
}nand_vector_t;

typedef struct _nand_op_t
{
    char            *buf;
    uint16          size;
    nand_vector_t   vector;
}nand_op_t;

void memory_dump_dword(char* type, void *buffer, uint32 size)
{
    printf("%s to check buffer:\n", type);
    for (uint32 i = 0; i < size; i++) {
        if (i % 32 == 0) {
            printf("\n");
        }
        printf(" %8x", ((uint32 *)buffer)[i]);
    }
    printf("\n");
}

ssize_t write_physical_page(mongoc_gridfs_t *gridfs, nand_op_t *nand_op)
{
    bson_string_t *bson_string;
    mongoc_gridfs_file_t *file;
    mongoc_gridfs_file_opt_t opt = {0};
    mongoc_iovec_t iov;
    ssize_t r;
    uint32 au_nr = nand_op->vector.field.au_nr;

    assert(nand_op->vector.field.au_off == 0);
    assert(au_nr == AU_NR_PER_PAGE);
    printf("start write\n");
    for (uint32 i = 0; i < au_nr; i++) {
        bson_string = bson_string_new (NULL);
        nand_op->vector.field.au_off = i;
        nand_op->vector.field.au_nr  = 1;
        bson_string_append_printf (bson_string, "%lx", nand_op->vector.value);
        opt.filename = bson_string->str;
        opt.chunk_size = AU_SIZE;
        file = mongoc_gridfs_create_file (gridfs, &opt);
        assert(file);
        iov.iov_base 	= (void *) (nand_op->buf + i * AU_SIZE);
        iov.iov_len		= AU_SIZE;
        memory_dump_dword("write", iov.iov_base, iov.iov_len >> 2);
        r = mongoc_gridfs_file_writev (file, &iov, 1, -1);
        bson_string_free (bson_string, true);
        mongoc_gridfs_file_save(file);
    }

    mongoc_gridfs_file_destroy (file);
    return r;
}

ssize_t read_physical_au(mongoc_gridfs_t *gridfs, nand_op_t *nand_op)
{
    bson_string_t *bson_string;
    mongoc_gridfs_file_t *file;
    mongoc_iovec_t iov;
    ssize_t r;
    bson_error_t error;
    mongoc_stream_t *stream;
    bson_string = bson_string_new (NULL);
    bson_string_append_printf (bson_string, "%lx", nand_op->vector.value);
    file = mongoc_gridfs_find_one_by_filename (gridfs, bson_string->str, &error);
    bson_string_free (bson_string, true);

    if (!file) {
        printf("read empty au\n");
        return 0;
    }

    stream = mongoc_stream_gridfs_new (file);
    assert (stream);

    iov.iov_base 	= (void *) (nand_op->buf + nand_op->vector.field.au_off	 * AU_SIZE);
    iov.iov_len		= AU_SIZE;
    r = mongoc_stream_readv (stream, &iov, 1, -1, 0);
    memory_dump_dword("read", iov.iov_base, iov.iov_len >> 2);
    assert (r >= 0);
    mongoc_stream_destroy (stream);

    mongoc_gridfs_file_destroy (file);
    return r;
}

int
main (int argc, char *argv[])
{
    mongoc_gridfs_t *gridfs;
    mongoc_client_t *client;
    const char *uri_string = "mongodb://127.0.0.1:27017/?appname=gridfs-example";
    mongoc_uri_t *uri;
    bson_error_t error;
    char buf[PAGE_SIZE];
    char *get_filename;
    nand_op_t nand_op;
    //1. prepare one mongodb database
    mongoc_init ();
    uri = mongoc_uri_new_with_error (uri_string, &error);
    if (!uri) {
        return EXIT_FAILURE;
    }
    client = mongoc_client_new_from_uri (uri);
    assert (client);
    mongoc_client_set_error_api (client, 2);
    gridfs = mongoc_client_get_gridfs (client, "test", "fs", &error);

    //2. to simulate physical address of dedicated NAND.
    nand_op.vector.field.ch		= 4;
    nand_op.vector.field.ce		= 2;
    nand_op.vector.field.lun	= 1;
    nand_op.vector.field.plane	= 0;
    nand_op.vector.field.block	= 0;
    nand_op.vector.field.page	= 0;
    nand_op.vector.field.au_off	= 0;
    nand_op.vector.field.au_nr	= AU_NR_PER_PAGE;
    nand_op.buf = buf;
    for (uint32 i = 0; i < AU_NR_PER_PAGE; i++) {
        memset(&buf[i * AU_SIZE], i, AU_SIZE);
    }
    printf("start test program/read compare\n");

    while (1) {
        nand_op.vector.field.au_off	= 0;
        nand_op.vector.field.au_nr	= AU_NR_PER_PAGE;
        write_physical_page(gridfs, &nand_op);

        for (uint32 i = 0; i < AU_NR_PER_PAGE; i++) {
            nand_op.vector.field.au_off	= i;
            nand_op.vector.field.au_nr	= 1;
            read_physical_au(gridfs, &nand_op);
        }
        nand_op.vector.field.page ++;
        if (nand_op.vector.field.page == 2) {
            mongoc_gridfs_destroy (gridfs);
            mongoc_uri_destroy (uri);
            mongoc_client_destroy (client);
            mongoc_cleanup ();
            assert(0);
        }
    }
    return EXIT_SUCCESS;
}
