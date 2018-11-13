#include <mongoc/mongoc.h>
#include "logical_lun_tester.h"
#include "simulator_tester.h"
#include "nand_vector_tester.h"
#include "testers.h"

void tester(mongoc_gridfs_t *gridfs)
{
    tester_initialization();
    mongodb_tester(gridfs);
    nand_vector_tester(gridfs);
    logical_lun_operator_self_test(gridfs);
}
