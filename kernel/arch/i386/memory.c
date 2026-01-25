#include "memory.h"
#include <stdio.h>

void init_memory(MultiBootInfo* info) {
    printf("MMap: \n");
    for (uint32_t i = 0; i < info->mmap_length; i += sizeof(MultiBootMmapEntry)) {
        MultiBootMmapEntry* mmmt = (MultiBootMmapEntry*)(info->mmap_addr + i);
        printf("    Laddr: %x|HAddr: %x|LLen: %x|HLen: %x|Size: %x|Type: %d\n", mmmt->addr_low, mmmt->addr_high, mmmt->len_low, mmmt->len_high, mmmt->size, mmmt->type);
    }
};
