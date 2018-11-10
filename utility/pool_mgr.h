#pragma once

#define MAX_POOL_NODE_SZ_BITS   16
#define MAX_POOL_NODE_SZ        (1UL << MAX_POOL_NODE_SZ_BITS)
#define MAX_POOL_ID_LEN         20
typedef struct _pool_node_t
{
    struct _pool_node_t *node_next;
} pool_node_t;

typedef struct _pool_op_t
{
    pool_node_t *free_start;
    pool_node_t *free_end;
} pool_op_t;

typedef struct _pool_mgr_t
{
    uint32      node_sz;
    pool_node_t *node_free;
    pool_op_t   pool_op;
    char        pool_id[MAX_POOL_ID_LEN];
} pool_mgr_t;

void pool_mgr(char* pool_id, pool_mgr_t *pool_mgr, uint32 node_size, void* pool_base, uint32 node_nr);
pool_node_t * pool_allocate_nodes(pool_mgr_t *pool_mgr, uint32 *got_nr, uint32 want_nr);
void pool_release_nodes(pool_mgr_t *pool_mgr, pool_node_t *push_start, pool_node_t *push_end);
