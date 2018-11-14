#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

void memory_dump_u64(char* type, uint32 cnt, void *buffer, uint32 size)
{
    for (uint32 i = 0; i < size; i++) {
        if (i % 32 == 0) {
            printf("\n");
        }
        printf(" %lx", ((uint64 *)buffer)[i]);
    }
    printf("\n");
}

uint8 *align_memory_malloc(uint32 size,uint32 align_bytes)
{
	uint8 *base_ptr = NULL;
	uint8 *mem_ptr = NULL;
	uint8 part_bytes;

	base_ptr = (uint8 *)malloc(size + align_bytes);
	assert(align_bytes <= 0xFF);
	assert((align_bytes & align_bytes - 1) == 0);

	part_bytes = align_bytes - ((uint32)base_ptr) & (align_bytes - 1);
	if (part_bytes == 0) {
		part_bytes = align_bytes;
	}
	mem_ptr = base_ptr + part_bytes;

	(((uint8*) mem_ptr) - 1)[0] = part_bytes;
	printf("actual %x allocate %x offset %d \n",(uint32)mem_ptr, (uint32)base_ptr, part_bytes);
	return mem_ptr;
}


void align_memory_free(uint8 *mem_ptr)
{
	uint8 *base_addr = NULL;
	base_addr = ((uint8*) mem_ptr) - (((uint8*) mem_ptr) - 1)[0];
	printf("actual %x release %x offset %d\n",(uint32)mem_ptr,(uint32)base_addr, (((uint8*) mem_ptr) - 1)[0]);
	free(base_addr);
}




