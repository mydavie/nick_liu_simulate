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

void nand_op_resource_alloc(nand_operator_t *pnand_operator)
{
    pnand_operator->buf = (char *)malloc(NAND_VECTOR_POOL_NR * AU_SIZE);
}

void nand_op_resource_release(nand_operator_t *pnand_operator)
{
    free(pnand_operator->buf);
}

void
nand_operator_self_test (mongoc_gridfs_t *gridfs)
{
    nand_operator_t nand_operator;

    nand_op_resource_alloc(&nand_operator);
    nand_operator.lun_paa_operator.start.field.au_of_lun    = 0;
    nand_operator.lun_paa_operator.start.field.llun         = 1;
    nand_operator.lun_paa_operator.start.field.pb_type      = SLC_OTF_TYPE;
    nand_operator.lun_paa_operator.start.field.psb          = 3;
    nand_operator.lun_paa_operator.cnt                      = 4;
    nand_operator.lun_paa_operator.list                     = NULL;
    nand_operator.op_type = OP_TYPE_PROGRAM_NORMAL;
    printf("start program compare\n");
    nand_operator_submit(gridfs, &nand_operator);
    nand_release_vectors(nand_operator.vector_operator.list_start, nand_operator.vector_operator.cnt);
    nand_operator.op_type = OP_TYPE_READ_NORMAL;
    printf("start read compare\n");
    nand_operator_submit(gridfs, &nand_operator);
    nand_release_vectors(nand_operator.vector_operator.list_start, nand_operator.vector_operator.cnt);
    nand_op_resource_release(&nand_operator);
}

void tester_initialization(void)
{
    NAND_initialization();
}
