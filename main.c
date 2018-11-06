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

#define KB_BITS                     4
#define KB                          (1 << KB_BITS)

//NAND physical layout bits limitation macro definition
#define MAX_AU_BITS                 (2 + KB_BITS)
#define MAX_PAGE_SIZE_BITS          (4 + KB_BITS)   //FW support max au offset per physical page
#define MAX_AU_PER_PAGE_BITS        (MAX_PAGE_SIZE_BITS - MAX_AU_BITS)
#define MAX_CH_BITS                 4   //FW support max physical CHANNEL number
#define MAX_CE_PER_CH_BITS          4   //FW support max physical CE number
#define MAX_LUN_PER_CE_BITS         2   //FW support max physical LUN number per physical ce
#define MAX_PLANE_PER_LUN_BITS      2   //FW support max physical PLANE number per physical lun
#define MAX_BLOCK_PER_PLANE_BITS    10  //FW support max physical BLOCK number per physical plane
#define MAX_PAGE_PER_BLOCK_BITS     14  //FW support max physical PAGE number per physical block

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


//the NAND layout physical address that used by ASIC, and transfered from PAA
#define NAND_VECTOR_POOL_NR     (AU_NR_PER_PAGE * 3)      //NAND layout operator poll
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
        uint64 rsvd     : 64 - MAX_NAND_LAYOUT_BITS;
	} field;
}nand_vector_t;

typedef struct _nand_op_t
{
    char            *buf;                                   //the DRAM address that save user data
    uint16          size;                                   //the user data size
    uint16          vector_cnt;
    nand_vector_t   *vector;                                 //the physical address of NAND that to save user data

}nand_op_t;

//the NAND layout logical address that used FTL, and will be translated into nand_vector_t
typedef struct _paa_t
{
    uint64 pbp          :   MAX_BLOCK_PER_PLANE_BITS;       //the supper block id, that composed by the same physical block id in different plane.
    uint64 au_offset    :   MAX_NAND_LAYOUT_BITS;           //the internal au offset of one supper block
}paa_t;

uint32 paa_list_to_nand_vectors(paa_t *paa, nand_vector_t *nand_vector)
{

    return true;
}

void memory_dump_dword(char* type, uint32 cnt, void *buffer, uint32 size)
{
    printf("%s au[%d]:\n", type, cnt);
    for (uint32 i = 0; i < size; i++) {
        if (i % 32 == 0) {
            printf("\n");
        }
        printf(" %8x", ((uint32 *)buffer)[i]);
    }
    printf("\n");
}

uint32 write_nand_vectors(mongoc_gridfs_t *gridfs, nand_op_t *nand_op)
{
    bson_string_t *bson_string;
    mongoc_gridfs_file_t *file;
    mongoc_gridfs_file_opt_t opt = {0};
    mongoc_iovec_t iov;
    ssize_t r;
    uint32 vector_cnt = nand_op->vector_cnt;

    printf("write vectors\n");
    for (uint32 i = 0; i < vector_cnt; i++) {
      bson_string = bson_string_new (NULL);
      bson_string_append_printf (bson_string, "%lx", nand_op->vector[i].value);
      opt.filename = bson_string->str;
      opt.chunk_size = AU_SIZE;
      file = mongoc_gridfs_create_file (gridfs, &opt);
      assert(file);
      iov.iov_base    = (void *) (nand_op->buf + i * AU_SIZE);
      iov.iov_len     = AU_SIZE;
      memory_dump_dword("write", i, iov.iov_base, iov.iov_len >> 2);
      r = mongoc_gridfs_file_writev (file, &iov, 1, -1);
      bson_string_free (bson_string, true);
      mongoc_gridfs_file_save(file);
    }

    mongoc_gridfs_file_destroy (file);
    return true;
}

uint32 read_nand_vectors(mongoc_gridfs_t *gridfs, nand_op_t *nand_op)
{
    bson_string_t *bson_string;
    mongoc_gridfs_file_t *file;
    mongoc_gridfs_file_opt_t opt = {0};
    mongoc_iovec_t iov;
    ssize_t r;
    uint32 vector_cnt = nand_op->vector_cnt;

    printf("read vectors\n");
    for (uint32 i = 0; i < vector_cnt; i++) {
      bson_string = bson_string_new (NULL);
      bson_string_append_printf (bson_string, "%lx", nand_op->vector[i].value);
      opt.filename = bson_string->str;
      opt.chunk_size = AU_SIZE;
      file = mongoc_gridfs_create_file (gridfs, &opt);
      assert(file);
      iov.iov_base    = (void *) (nand_op->buf + i * AU_SIZE);
      iov.iov_len     = AU_SIZE;
      memory_dump_dword("read", i, iov.iov_base, iov.iov_len >> 2);
      r = mongoc_gridfs_file_writev (file, &iov, 1, -1);
      bson_string_free (bson_string, true);
      mongoc_gridfs_file_save(file);
    }

    mongoc_gridfs_file_destroy (file);
    return true;
}

int
main (int argc, char *argv[])
{
    mongoc_gridfs_t *gridfs;
    mongoc_client_t *client;
    const char *uri_string = "mongodb://127.0.0.1:27017/?appname=gridfs-example";
    mongoc_uri_t *uri;
    bson_error_t error;
    nand_op_t nand_op;
    //1. prepare one mongoDB database
    mongoc_init ();
    uri = mongoc_uri_new_with_error (uri_string, &error);
    if (!uri) {
        return EXIT_FAILURE;
    }
    client = mongoc_client_new_from_uri (uri);
    assert (client);
    mongoc_client_set_error_api (client, 2);
    gridfs = mongoc_client_get_gridfs (client, "test", "fs", &error);
    nand_op.vector = (nand_vector_t *)malloc(NAND_VECTOR_POOL_NR * sizeof(nand_vector_t));
    memset(nand_op.vector, 0, NAND_VECTOR_POOL_NR * sizeof(nand_vector_t));
    nand_op.buf     = (char *)malloc(NAND_VECTOR_POOL_NR * AU_SIZE);
    nand_op.vector[0].field.page   = 0;
    nand_op.vector[0].field.au_off = 0;
    uint32 au_cnt = 0;
    do {
        nand_op.vector[au_cnt].field.ch     = 4;
        nand_op.vector[au_cnt].field.ce     = 2;
        nand_op.vector[au_cnt].field.lun    = 1;
        nand_op.vector[au_cnt].field.plane  = 0;
        nand_op.vector[au_cnt].field.block  = 0;
        memset(nand_op.buf + au_cnt * AU_SIZE, (nand_op.vector[au_cnt].field.page << MAX_AU_PER_PAGE_BITS) | nand_op.vector[au_cnt].field.au_off, AU_SIZE);
        memory_dump_dword("fill buffer", au_cnt, nand_op.buf + au_cnt * AU_SIZE, AU_SIZE >> 2);
        if (nand_op.vector[au_cnt].field.au_off == (AU_NR_PER_PAGE - 1)) {
            nand_op.vector[au_cnt + 1].field.page   = nand_op.vector[au_cnt].field.page + 1;
            nand_op.vector[au_cnt + 1].field.au_off = 0;
        }
        else {
            nand_op.vector[au_cnt + 1].field.page   = nand_op.vector[au_cnt].field.page;
            nand_op.vector[au_cnt + 1].field.au_off = nand_op.vector[au_cnt].field.au_off + 1;
        }

        au_cnt++;
    } while (au_cnt < NAND_VECTOR_POOL_NR);

    nand_op.vector_cnt = NAND_VECTOR_POOL_NR;

    printf("start test program/read compare\n");

    while (1) {
        write_nand_vectors(gridfs, &nand_op);
        read_nand_vectors(gridfs, &nand_op);
        free(nand_op.vector);
        free(nand_op.buf);
        mongoc_gridfs_destroy (gridfs);
        mongoc_uri_destroy (uri);
        mongoc_client_destroy (client);
        mongoc_cleanup ();
        assert(0);
    }
    return EXIT_SUCCESS;
}
