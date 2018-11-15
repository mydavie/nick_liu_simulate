#include "types.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "types.h"
#include "utility.h"
#include "pool_mgr.h"
#include "memory_mgr.h"

pool_mgr_t memory_node_pool_mgr;

void memory_pool_init(uint32 node_nr,uint32 node_size, uint32 buffer_size)
{
	void *memory_pool = align_memory_malloc(node_nr * node_size, node_size);//depend on the memory align requirement
	pool_mgr("MEMORY NODE", &memory_node_pool_mgr, node_size, memory_pool, node_nr);
	memory_node_t* pnode = (memory_node_t*)memory_node_pool_mgr.node_free;

	for (uint32 i = 0; i < node_nr; i++)
	{
		pnode->buf 	= align_memory_malloc(buffer_size, 64);
		assert(pnode->buf);
		printf("poolID %s memory node 0x%lx buf 0x%lx  \n",
				memory_node_pool_mgr.pool_id, (uint64)pnode, (uint64)pnode->buf);
		pnode 		= (memory_node_t*)pnode->next;
	}
}

memory_node_t* memory_allcoate_nodes(uint32 want_nr, uint32 *result_nr)
{
	memory_node_t* pmemory_node = NULL;

	pmemory_node = (memory_node_t*)pool_allocate_nodes(&memory_node_pool_mgr, result_nr, want_nr);

    return pmemory_node;
}

uint32 memory_release_nodes(memory_node_t* start, uint32 vector_cnt)
{
    assert(vector_cnt);
    pool_release_nodes(&memory_node_pool_mgr, (pool_node_t *)start, vector_cnt);
    return true;
}

memory_node_t *find_next_node_segment(memory_node_t * pnode_buf, uint32 node_nr)
{
	while(node_nr--) {
		assert(pnode_buf);
		pnode_buf = (memory_node_t *)pnode_buf->next;
	}
	return pnode_buf;
}
