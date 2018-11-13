#include <mongoc/mongoc.h>
#include "logical_lun_tester.h"
#include "simulator_tester.h"
#include "nand_vector_tester.h"
#include "testers.h"
#include "../nand_vectors/nand_vectors.h"
#include "../nand_virtualize/logical_lun_operator.h"

void tester(mongoc_gridfs_t *gridfs)
{
    nand_init_onetime();
    logical_lun_init_onetime();

    mongodb_tester(gridfs);
    nand_vector_tester(gridfs);
    logical_lun_operator_self_test(gridfs);
}
