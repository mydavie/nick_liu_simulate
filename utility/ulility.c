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
	printf("to allocate memory ");
	base_ptr = (uint8 *)malloc(size + align_bytes);
	if (base_ptr == NULL) {
		printf("memory allocate fail %d\n", size + align_bytes);
		return base_ptr;
	}
	assert(align_bytes <= 0xFF);
	assert((align_bytes & align_bytes - 1) == 0);

	part_bytes = align_bytes - ((uint32)base_ptr) & (align_bytes - 1);
	if (part_bytes == 0) {
		part_bytes = align_bytes;
	}
	mem_ptr = base_ptr + part_bytes;

	(((uint8*) mem_ptr) - 1)[0] = part_bytes;
	printf("actual %lx allocate %lx offset %d size %d\n",(uint64)mem_ptr, (uint64)base_ptr, part_bytes, size);
	return mem_ptr;
}


void align_memory_free(uint8 *mem_ptr)
{
	uint8 *base_addr = NULL;
	printf("to free memory ");
	base_addr = ((uint8*) mem_ptr) - (((uint8*) mem_ptr) - 1)[0];
	printf("actual %lx release %lx offset %d\n",(uint64)mem_ptr,(uint64)base_addr, (((uint8*) mem_ptr) - 1)[0]);
	free(base_addr);
}




