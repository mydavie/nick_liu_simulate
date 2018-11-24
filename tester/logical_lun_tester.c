
#include "../lib_header.h"
#include "../nand_vectors/nand_vectors.h"
#include "../nand_virtualize/logical_lun_operator.h"
#include "logical_lun_tester.h"

void logical_lun_operator_self_test(mongoc_gridfs_t *gridfs)
{
    logical_lun_operator_t logical_lun_operator;
    uint32 want_nr = 1;
    uint32 result_nr = 0;
    logical_lun_t* plogical_lun = logical_lun_allocate(want_nr, &result_nr);
    logical_lun_operator.simulator = gridfs;

    if (want_nr == result_nr) {
        logical_lun_operator.list               = plogical_lun;
        logical_lun_operator.cnt                = result_nr;
        logical_lun_operator.op_type            = LOGICAL_LUN_OP_TYPE_PROGRAM;
        plogical_lun->au_param.range.au_start[0]   = random() % 20;
        plogical_lun->au_param.range.au_cnt[0]     = 1 + random() % 20;
        plogical_lun->valid_au_range_cnt        = 1;
        plogical_lun->outstanding_au_rang_cnt   = 0;
        plogical_lun->buf_node                  = memory_allcoate_nodes(plogical_lun->au_param.range.au_cnt[0], &result_nr);
        assert(plogical_lun->au_param.range.au_cnt[0] == result_nr);
        plogical_lun->llun_nand_type            = NATIVE_TYPE;
        plogical_lun->logical_lun_id            = 4;
        plogical_lun->llun_spb_id               = 34;
        plogical_lun->simulator                 = gridfs;
        plogical_lun->au_param_type             = LOGICAL_LUN_PARAM_RANGE;
        submit_logical_lun_operator(&logical_lun_operator);
        logical_lun_operator.op_type        = LOGICAL_LUN_OP_TYPE_READ;
        submit_logical_lun_operator(&logical_lun_operator);
        nand_release_vectors(plogical_lun->nand_vector, plogical_lun->vector_cnt);
        memory_release_nodes(plogical_lun->buf_node, result_nr);
    }
    else {
        logical_lun_release(plogical_lun, result_nr);
    }
}
