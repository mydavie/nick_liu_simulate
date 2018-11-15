#pragma once

typedef struct _memory_node_t
{
	pool_node_t *next;
	uint8 *buf;
}memory_node_t;

void memory_pool_init(uint32 node_nr,uint32 node_size, uint32 buffer_size);
memory_node_t* memory_allcoate_nodes(uint32 want_nr, uint32 *result_nr);
uint32 memory_release_nodes(memory_node_t* start, uint32 vector_cnt);
memory_node_t *find_next_node_segment(memory_node_t * pnode_buf, uint32 node_nr);
