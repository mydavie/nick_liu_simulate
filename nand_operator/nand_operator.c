#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <bson.h>
#include <stdio.h>
#include <locale.h>
#include <memory.h>
#include <assert.h>
#include <mongoc/mongoc.h>
#include "../utility/types.h"
#include "../utility/utility.h"
#include "nand_operator.h"
nand_vector_t l2p_lun_table[MAX_CHANNEL_NR + MAX_CE_PER_CHANNEL + MAX_LUN_PER_CE];

nand_info_t gnand_info = {0};


//the info will get from NAND parameter page.
uint32 nand_info_initialization(nand_info_t *pnand_info)
{
    pnand_info->ch_nr         = 8;
    pnand_info->ce_nr         = 4;
    pnand_info->block_nr      = 512;
    pnand_info->lun_nr        = 2;
    pnand_info->plane_nr      = 2;
    pnand_info->page_nr       = 1024;
    pnand_info->au_nr         = 4;
    pnand_info->cell_bits_nr  = 3;
    assert(pnand_info->ch_nr      < MAX_CHANNEL_NR);
    assert(pnand_info->ce_nr      < MAX_CE_PER_CHANNEL);
    assert(pnand_info->block_nr   < MAX_BLOCK_PER_PLANE);
    assert(pnand_info->lun_nr     < MAX_LUN_PER_CE);
    assert(pnand_info->plane_nr   < MAX_PLANE_PER_LUN);
    assert(pnand_info->page_nr    < MAX_PAGE_PER_BLOCK);

    return true;
}
uint32 l2p_lun_table_initialization(void)
{
    nand_info_t *pnand_info     = &gnand_info;
    uint32 llun_nr              = pnand_info->ch_nr * pnand_info->ce_nr * pnand_info->lun_nr;
    memset(l2p_lun_table, 0, sizeof (nand_vector_t) * (MAX_CHANNEL_NR + MAX_CE_PER_CHANNEL + MAX_LUN_PER_CE));

    for (uint32 i = 0; i < llun_nr; i++) {
        if (l2p_lun_table[i].field.plun.field.ch == pnand_info->ch_nr - 1) {
            if (l2p_lun_table[i].field.plun.field.ce == pnand_info->ce_nr - 1) {
                if (l2p_lun_table[i].field.plun.field.lun == pnand_info->lun_nr - 1) {
                    printf("init_l2p_lun_table init done\n");
                }
                else {
                    l2p_lun_table[i].field.plun.field.lun++;
                }
            }
            else {
                l2p_lun_table[i].field.plun.field.ce++;
            }
        }
        else {
            l2p_lun_table[i].field.plun.field.ch++;
        }
    }
    return true;
}

void NAND_initialization(void)
{
    nand_info_initialization(&gnand_info);
    l2p_lun_table_initialization();
}

uint32 write_nand_vectors(mongoc_gridfs_t *gridfs, nand_op_t *nand_op)
{
    bson_string_t *bson_string;
    mongoc_gridfs_file_t *file;
    mongoc_gridfs_file_opt_t opt = {0};
    mongoc_iovec_t iov;
    ssize_t r;
    uint32 vector_cnt = nand_op->op_cnt;

    printf("write vectors\n");
    for (uint32 i = 0; i < vector_cnt; i++) {
      bson_string = bson_string_new (NULL);
      bson_string_append_printf (bson_string, "%lx", nand_op->vector_list[i].value);
      opt.filename = bson_string->str;
      opt.chunk_size = AU_SIZE;
      file = mongoc_gridfs_create_file (gridfs, &opt);
      assert(file);
      iov.iov_base    = (void *) (nand_op->buf + i * AU_SIZE);
      iov.iov_len     = AU_SIZE;
      memset(iov.iov_base, i, AU_SIZE);
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
    uint32 vector_cnt = nand_op->op_cnt;

    printf("read vectors\n");
    for (uint32 i = 0; i < vector_cnt; i++) {
      bson_string = bson_string_new (NULL);
      bson_string_append_printf (bson_string, "%lx", nand_op->vector_list[i].value);
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

uint32 lun_paa_list_to_nand_vectors(lun_paa_t *paa, nand_vector_t *nand_vector, uint32 cnt)
{
    uint32 i                = 0;
    nand_info_t *pnand_info = &gnand_info;
    uint32 au_nr_per_page   = pnand_info->au_nr;
    uint32 au_nr_per_plane  = (paa[i].field.pb_type == SLC_OTF_TYPE) ? au_nr_per_page : (au_nr_per_page * pnand_info->cell_bits_nr);

    assert ((au_nr_per_plane & (au_nr_per_plane - 1)) == 0);
    do {
        nand_vector[i].value        = l2p_lun_table[paa[i].field.llun].value;
        nand_vector[i].field.block  = paa->field.psb;
        nand_vector[i].field.plane  = paa[i].field.au_of_lun & (au_nr_per_plane - 1);
        nand_vector[i].field.au_off = paa[i].field.au_of_lun & (au_nr_per_page - 1);
        i++;
        printf("paa: psb %d au_of_lun %d nand_vector: ch %d ce %d lun %d plane %d block %d au_off %d\n",
                paa->field.psb, paa[i].field.au_of_lun,
                nand_vector[i].field.plun.field.ch,
                nand_vector[i].field.plun.field.ce,
                nand_vector[i].field.plun.field.lun,
                nand_vector[i].field.plane,
                nand_vector[i].field.block,
                nand_vector[i].field.au_off);

    } while(i < cnt);

    return true;
}

uint32 lun_paa_range_to_nand_vectors(lun_paa_t *paa_start, nand_vector_t *nand_vector, uint32 cnt)
{
    uint32 i                    = 0;
    nand_info_t *pnand_info     = &gnand_info;
    uint32 au_nr_per_page       = pnand_info->au_nr;
    uint32 au_nr_per_plane      = (paa_start[0].field.pb_type == SLC_OTF_TYPE) ? au_nr_per_page : (au_nr_per_page * pnand_info->cell_bits_nr);
    uint32 au_nr_per_plane_bits = __builtin_ctz(au_nr_per_plane);

    assert ((au_nr_per_plane & (au_nr_per_plane - 1)) == 0);
    do {
        nand_vector[i].value        = l2p_lun_table[paa_start[0].field.llun].value;
        nand_vector[i].field.block  = paa_start[0].field.psb;
        nand_vector[i].field.plane  = paa_start[0].field.au_of_lun >> au_nr_per_plane_bits;
        nand_vector[i].field.au_off = paa_start[0].field.au_of_lun & (au_nr_per_page - 1);

        printf("paa: psb %d au_of_lun %d nand_vector: ch %d ce %d lun %d plane %d block %d au_off %d\n",
                paa_start[0].field.psb, paa_start[0].field.au_of_lun,
                nand_vector[i].field.plun.field.ch,
                nand_vector[i].field.plun.field.ce,
                nand_vector[i].field.plun.field.lun,
                nand_vector[i].field.plane,
                nand_vector[i].field.block,
                nand_vector[i].field.au_off);
        paa_start[0].field.au_of_lun++;
        i++;

    } while(i < cnt);

    return true;
}

