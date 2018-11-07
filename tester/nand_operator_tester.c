#include <assert.h>
#include <mongoc/mongoc.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <bson.h>
#include <stdio.h>
#include <locale.h>
#include <memory.h>
#include "nand_operator_tester.h"

#include "../nand_operator/nand_operator.h"
#include "../utility/types.h"
#include "../utility/utility.h"

void nand_op_resource_alloc(nand_op_t *pnand_op)
{
    pnand_op->vector_list   = (nand_vector_t *)malloc(NAND_VECTOR_POOL_NR * sizeof(nand_vector_t));
    pnand_op->lun_paa.list  = (lun_paa_t *)malloc(NAND_VECTOR_POOL_NR * sizeof(lun_paa_t));
    pnand_op->buf           = (char *)malloc(NAND_VECTOR_POOL_NR * AU_SIZE);
    memset(pnand_op->lun_paa.list, 0, NAND_VECTOR_POOL_NR * sizeof(lun_paa_t));
    memset(pnand_op->vector_list, 0, NAND_VECTOR_POOL_NR * sizeof(nand_vector_t));
}

void nand_op_resource_release(nand_op_t *pnand_op)
{
    free(pnand_op->vector_list);
    free(pnand_op->lun_paa.list);
    free(pnand_op->buf);
}

void vector_self_test (mongoc_gridfs_t *gridfs)
{
    nand_op_t nand_op;

    nand_op_resource_alloc(&nand_op);
    nand_op.vector_list[0].field.page   = 0;
    nand_op.vector_list[0].field.au_off = 0;
    uint32 au_cnt = 0;

    do {
          nand_op.vector_list[au_cnt].field.plun.field.ch     = 4;
          nand_op.vector_list[au_cnt].field.plun.field.ce     = 2;
          nand_op.vector_list[au_cnt].field.plun.field.lun    = 1;
          nand_op.vector_list[au_cnt].field.plane  = 0;
          nand_op.vector_list[au_cnt].field.block  = 0;
          memset(nand_op.buf + au_cnt * AU_SIZE, (nand_op.vector_list[au_cnt].field.page << MAX_AU_PER_PAGE_BITS) | nand_op.vector_list[au_cnt].field.au_off, AU_SIZE);
          memory_dump_dword("fill buffer", au_cnt, nand_op.buf + au_cnt * AU_SIZE, AU_SIZE >> 2);
          if (nand_op.vector_list[au_cnt].field.au_off == (AU_NR_PER_PAGE - 1)) {
              nand_op.vector_list[au_cnt + 1].field.page   = nand_op.vector_list[au_cnt].field.page + 1;
              nand_op.vector_list[au_cnt + 1].field.au_off = 0;
          }
          else {
              nand_op.vector_list[au_cnt + 1].field.page   = nand_op.vector_list[au_cnt].field.page;
              nand_op.vector_list[au_cnt + 1].field.au_off = nand_op.vector_list[au_cnt].field.au_off + 1;
          }

          au_cnt++;
      } while (au_cnt < NAND_VECTOR_POOL_NR);

    nand_op.op_cnt = NAND_VECTOR_POOL_NR;


    lun_paa_range_to_nand_vectors(&nand_op.lun_paa.start, nand_op.vector_list, nand_op.op_cnt);

    printf("start test program/read compare\n");

    write_nand_vectors(gridfs, &nand_op);
    read_nand_vectors(gridfs, &nand_op);
    nand_op_resource_release(&nand_op);
}

void
lun_paa_self_test (mongoc_gridfs_t *gridfs)
{
    nand_op_t nand_op;

    nand_op_resource_alloc(&nand_op);
    nand_op.vector_list[0].field.page   = 0;
    nand_op.vector_list[0].field.au_off = 0;

    nand_op.op_cnt = NAND_VECTOR_POOL_NR;
    //lun_paa_list_to_nand_vectors(nand_op.lun_paa.list, nand_op.vector_list, 1);
    nand_op.lun_paa.start.field.pb_type     = SLC_OTF_TYPE;
    nand_op.lun_paa.start.field.llun        = 0;
    nand_op.lun_paa.start.field.au_of_lun   = 0;
    lun_paa_range_to_nand_vectors(&nand_op.lun_paa.start, nand_op.vector_list, nand_op.op_cnt);

    printf("start test program/read compare\n");

    write_nand_vectors(gridfs, &nand_op);
    read_nand_vectors(gridfs, &nand_op);
    nand_op_resource_release(&nand_op);
}

void tester_initialization(void)
{
    NAND_initialization();
}
