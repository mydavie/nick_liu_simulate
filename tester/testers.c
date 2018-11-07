#include <mongoc/mongoc.h>
#include "nand_operator_tester.h"
#include "testers.h"

void tester(mongoc_gridfs_t *gridfs)
{
    tester_initialization();
    //vector_self_test (gridfs);
    lun_paa_self_test (gridfs);
}
