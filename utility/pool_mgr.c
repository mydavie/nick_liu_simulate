#include "types.h"
#include <stdio.h>
#include "pool_mgr.h"
#include <string.h>
#include <assert.h>

typedef union _endian_check_t
{
	uint8  byte;
	uint32 word;
}endian_check_t;

void pool_mgr(char* pool_id, pool_mgr_t *pool_mgr, uint32 node_size, void* pool_base, uint32 node_nr)
{
    pool_node_t *pool_node  = (pool_node_t*) pool_base;
    pool_mgr->node_sz       = node_size;
    pool_mgr->node_free     = pool_base;
    pool_mgr->init_cnt      = node_nr;

    assert(strlen (pool_id) < MAX_POOL_ID_LEN);
    endian_check_t endian_check;
    endian_check.word = pool_id[0];
    //ensure the code work on little endian mode.
    assert(endian_check.word == endian_check.byte);

    memcpy(pool_mgr->pool_id, pool_id, strlen(pool_id));

    for (uint32 i = 0; i < node_nr; i++)
    {
        if (i == node_nr - 1) {
            pool_node->node_next    = NULL;
        }
        else {
            pool_node->node_next    = (pool_node_t*)((uint8 *)pool_node + node_size);
        }
        printf("poolID %s pool_base 0x%x [node i %d nr %d size 0x%x]  node address 0x%x next 0x%x\n",
                pool_mgr->pool_id, (uint32)pool_base, i , node_nr, node_size, (uint32)pool_node, (uint32)pool_node->node_next);

        pool_node               = pool_node->node_next;
    }
}

pool_node_t * pool_allocate_nodes(pool_mgr_t *pool_mgr, uint32 *got_nr, uint32 want_nr)
{
    pool_node_t* start_free_node    = pool_mgr->node_free;
    pool_node_t* cur_free_node      =  start_free_node;
    uint32 got_cnt              	= 0;

    while (cur_free_node)
    {
        got_cnt++;
        pool_mgr->allocate_cnt++;
        printf("pool allocated node 0x%x cnt %d from poolID %s\n", (uint32)cur_free_node, got_cnt, pool_mgr->pool_id);
        cur_free_node      =  cur_free_node->node_next;

        if ((got_cnt == want_nr) || (cur_free_node == NULL))
        {
            break;
        }
    }
    pool_mgr->node_free = cur_free_node;
    *got_nr             = got_cnt;

    return start_free_node;
}

void pool_release_nodes(pool_mgr_t *pool_mgr, pool_node_t *push_start, uint32 cnt)
{
	pool_node_t *push_end = NULL;
	assert(pool_mgr->allocate_cnt >= cnt);
	pool_mgr->allocate_cnt -= cnt;
	push_end = push_start + pool_mgr->node_sz * (cnt - 1);
    printf("pool release node start 0x%x end %d cnt %d to pool %s [t:%d a: %d]\n",
    		(uint32)push_start, (uint32)push_end, cnt, pool_mgr->pool_id, pool_mgr->init_cnt, pool_mgr->allocate_cnt);
    push_end->node_next = pool_mgr->node_free;
    pool_mgr->node_free = push_start;

}

