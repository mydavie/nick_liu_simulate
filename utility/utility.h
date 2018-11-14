#pragma once
void memory_dump_u64(char* type, uint32 cnt, void *buffer, uint32 size);
void *align_memory_malloc(uint32 size,uint32 align_bytes);
void align_memory_free(uint8 *mem_ptr);
