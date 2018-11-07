#include "types.h"
#include <stdio.h>

void memory_dump_dword(char* type, uint32 cnt, void *buffer, uint32 size)
{
    printf("%s au[%d]:\n", type, cnt);
    for (uint32 i = 0; i < size; i++) {
        if (i % 32 == 0) {
            printf("\n");
        }
        printf(" %8x", ((uint32 *)buffer)[i]);
    }
    printf("\n");
}
