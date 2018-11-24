#include "../lib_header.h"
#include "../nand_vectors/nand_vectors.h"
#include "../nand_virtualize/logical_lun_operator.h"
#include "logical_lun_tester.h"
#include "simulator_tester.h"
#include "nand_vector_tester.h"
#include "nand_allocator_tester.h"

void tester(mongoc_gridfs_t *gridfs)
{
	memory_pool_init(20, sizeof(memory_node_t), AU_SIZE);
    nand_init_onetime();
    logical_lun_init_onetime();

//    for (uint32 i = 0; i < 10; i ++){
//		mongodb_tester(gridfs);
//    }
//    for (uint32 i = 0; i < 10; i ++){
//  	   nand_vector_tester(gridfs);
//    }
//    for (uint32 i = 0; i < 10; i ++){
//    	logical_lun_operator_self_test(gridfs);;
//    }
    nand_allocator_tester(gridfs);
}
