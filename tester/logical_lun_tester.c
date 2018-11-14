#include <assert.h>
#include <mongoc/mongoc.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <bson.h>
#include <stdio.h>
#include <locale.h>
#include <memory.h>
#include "../nand_vectors/nand_vectors.h"
#include "../utility/types.h"
#include "../utility/utility.h"
#include "logical_lun_tester.h"
#include "../nand_virtualize/logical_lun_operator.h"

void logical_lun_operator_self_test(mongoc_gridfs_t *gridfs)
{
    logcial_lun_operator_t logical_lun_operator;
    uint32 want_nr = 1;
    uint32 result_nr = 0;

    logical_lun_t* plogical_lun = logical_lun_allcoate(want_nr, &result_nr);


    if (want_nr == result_nr) {
        logical_lun_operator.list               = plogical_lun;
        logical_lun_operator.cnt                = result_nr;
        logical_lun_operator.op_type            = LOGICAL_LUN_OP_TYPE_PROGRAM;
        plogical_lun->au_param.range.au_start   = 5;
        plogical_lun->au_param.range.au_cnt     = 8;
        plogical_lun->buf                   	= align_memory_malloc(plogical_lun->au_param.range.au_cnt * AU_SIZE, 64);
        plogical_lun->llun_nand_type        	= NATIVE_TYPE;
        plogical_lun->llun_offset           	= 4;
        plogical_lun->llun_spb_id           	= 34;
        plogical_lun->simulator_ptr         	= gridfs;

        plogical_lun->au_param_type         = LOGICAL_LUN_PARAM_RANGE;
        submit_logical_lun_operator(&logical_lun_operator);
        logical_lun_operator.op_type        = LOGICAL_LUN_OP_TYPE_READ;
        submit_logical_lun_operator(&logical_lun_operator);
        free(plogical_lun->buf);
    }
    else {
        logical_lun_release(plogical_lun, result_nr);
    }
}
