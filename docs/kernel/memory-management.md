# Memory Management Deep Dive

This guide explains TilekarOS's three-layered memory management stack, spanning from physical hardware frames to user-space heap allocation.

## 1. Physical Memory Manager (PMM)
**Source File**: [memory.c](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/mm/memory.c){: target="_blank" }

The PMM is the lowest layer of memory management. It treats the entire 4GB RAM as a series of 4KB "frames."

### Bitmap-Based Allocation
TilekarOS uses a **Bitmap** (`physical_memory_bitmap`) to track free frames. Each bit represents one 4KB frame:
- **0**: Free
- **1**: Allocated

??? example "Code Preview: `memory.c` (PMM Implementation)"
    ```c
    <--8<-- "kernel/arch/i386/mm/memory.c"
    ```

!!! tip "Why we use a bitmap"
    A bitmap is memory-efficient and easy to implement. For 4GB of RAM, the bitmap only takes **128 KB** of space.

---

## 2. Virtual Memory Manager (VMM) & Paging
**Source File**: [memory.c](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/mm/memory.c){: target="_blank" }

The VMM manages **32-bit Paging**, allowing the kernel to map virtual addresses to physical ones.

### Higher-Half Memory Layout

```mermaid
block-beta
    columns 1
    
    block:recursive["Recursive Mapping (0xFFC00000 - 0xFFFFFFFF)"]
        columns 1
        pd["Page Directory (0xFFFFF000)"]
        pt["Page Tables (0xFFC00000)"]
    end

    block:kernel_space["Kernel Space (0xC0000000 - 0xFFBFFFFF)"]
        columns 1
        heap["Kernel Heap (0xD0000000+)"]
        kcode["Kernel Code & Data (0xC0100000+)"]
    end

    block:user_space["User Space (0x00000000 - 0xBFFFFFFF)"]
        columns 1
        ustack["User Stack"]
        ucode["User Code & Data"]
    end
```

### Memory Layout Details

```mermaid
block-beta
    columns 1
    
    block:high_mem
        space
        text["High Memory (Available for Allocation)"]
        space
    end

    block:kernel_space_inner
        block:sections
            columns 3
            stack["Kernel Stack<br>(16 KB)"]
            bss[".bss Section<br>(Uninitialized)"]
            data[".data Section<br>(Initialized)"]
        end
        code[".text Section<br>(Kernel Code)"]
    end

    block:reserved
        vga["VGA Video Memory<br>(0xB8000 - 0xBFFFF)"]
        bios["BIOS Data Area & EBDA<br>(Reserved)"]
    end
```

### Recursive Paging
TilekarOS implements **Recursive Mapping** for its Page Directory. The very last entry in the Page Directory (index 1023) points back to the Page Directory itself. See [memory.h](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/mm/memory.h){: target="_blank" } for the relevant constants.

This trick allows the kernel to access and modify page tables using special virtual addresses without needing to constantly map and unmap them.

- **`RECURSIVE_PAGE_DIR`**: `0xFFFFF000`
- **`RECURSIVE_PAGE_TABLE(i)`**: `0xFFC00000 + (i << 12)`

```mermaid
graph TD
    VA[Virtual Address] --> VMM{VMM}
    VMM -->|Lookup PD| PD[Page Directory]
    PD -->|Lookup PT| PT[Page Table]
    PT -->|Frame Address| PA[Physical Memory]
    PA -->|Return Data| VA
```

### Physical Address Translation
The kernel provides a `memory_get_phys(uintptr_t vaddr)` function. This is critical for **DMA** and low-level drivers that need to provide the actual physical memory addresses to hardware devices like the ATA controller.

!!! info "OSDev Reference"
    For details on recursive mapping, see [OSDev: Recursive Paging](https://wiki.osdev.org/Recursive_Paging).

---

## 3. Kernel Heap (`kmalloc`)
**Source File**: [kmalloc.c](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/mm/kmalloc.c){: target="_blank" }

The heap provides dynamic allocation for kernel objects using a **First-Fit Linked List** strategy.

### Allocation Process (`kmalloc`):
1.  **First-Fit Search**: Scans the `block_header_t` list for a free block that is large enough.
2.  **Splitting**: If a block is much larger than requested, it is split into two, leaving the remainder free.
3.  **Heap Extension (`heap_sbrk`)**: If no suitable block is found, `kmalloc` requests more physical frames from the PMM and maps them into the virtual heap space (`0xD0000000`).

??? example "Code Preview: `kmalloc.c`"
    ```c
    <--8<-- "kernel/arch/i386/mm/kmalloc.c"
    ```

### Deallocation (`kfree`):
- **Coalescing**: When a block is freed, it is merged with its neighbors (if they are also free) to reduce fragmentation.

---

## 4. User-Space Heap Management

**Source File**: [malloc.c](https://github.com/SohamTilekar/TilekarOS/blob/main/libc/stdlib/malloc.c){: target="_blank" }

User-space applications manage their own heap using `malloc`, `calloc`, `realloc`, and `free` functions backed by the `SYS_BRK` syscall.

### Process Break (SYS_BRK)

The `brk()` and `sbrk()` syscalls manage the process break—the boundary between the heap and unallocated memory:

```c
int brk(void* addr);         // Set break to exact address
void* sbrk(intptr_t inc);    // Increment break by inc bytes
```

### User-Space Allocator Strategy

TilekarOS uses a **First-Fit Allocator** for user-space:

1. **Allocation (`malloc`)**:
   - Maintains a linked list of free blocks with headers
   - Searches for first block large enough for requested size
   - Splits blocks if remainder is significant
   - Calls `sbrk()` to extend heap if needed

2. **Deallocation (`free`)**:
   - Marks block as free
   - Coalesces adjacent free blocks to reduce fragmentation

3. **Variants**:
   - `calloc(nmemb, size)`: Allocates and zero-initializes
   - `realloc(ptr, size)`: Resizes existing allocation

### Memory Layout (User Process)

```
High Address
+-------------------+
|                   |
|   Stack (grows ↓) |
+-------------------+
|                   |
|                   |  Unallocated
|                   |
+-------------------+ <- Process Break (SYS_BRK)
|                   |
|   Heap (grows ↑)  |  malloc/free managed
|                   |
+-------------------+ <- Heap Start
| .bss Section      |
+-------------------+
| .data Section     |
+-------------------+
| .text Section     |
+-------------------+
Low Address
```

### Example: User-Space Allocation

```c
#include <stdlib.h>
#include <stdio.h>

int main() {
    // Allocate array
    int* data = (int*)malloc(1000 * sizeof(int));
    if (data == NULL) {
        printf("malloc failed!\n");
        return 1;
    }
    
    // Use array
    for (int i = 0; i < 1000; i++) {
        data[i] = i * 2;
    }
    printf("Sum: %d\n", data[500]);
    
    // Resize
    int* new_data = (int*)realloc(data, 2000 * sizeof(int));
    if (new_data) {
        data = new_data;
        printf("Resized to 2000 elements\n");
    }
    
    // Cleanup
    free(data);
    return 0;
}
```

### Allocator Metadata

Each allocation includes a header tracking allocation size:

```c
typedef struct {
    size_t size;
    unsigned char magic;  // Corruption detection
} malloc_header_t;

// Memory layout:
// [malloc_header_t] [user data...]
```

---

## 5. Test/Example: Stress-Testing the Heap

You can verify the heap's correctness by allocating large chunks and then freeing them to ensure coalescing works:

```c
void test_heap() {
    // 1. Allocate 100 small chunks
    void* ptrs[100];
    for (int i = 0; i < 100; i++) {
        ptrs[i] = kmalloc(64);
    }

    // 2. Free alternating chunks (Creating gaps)
    for (int i = 0; i < 100; i += 2) {
        kfree(ptrs[i]);
    }

    // 3. Allocate a larger chunk (Should trigger coalescing)
    void* big = kmalloc(512);
    kfree(big);

    // 4. Free the rest
    for (int i = 1; i < 100; i += 2) {
        kfree(ptrs[i]);
    }
}
```

### User-Space Heap Test

```c
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main() {
    printf("=== User-Space Heap Test ===\n");
    
    // Test 1: Basic allocation
    printf("Test 1: Basic allocation...\n");
    int* arr1 = (int*)malloc(10 * sizeof(int));
    if (arr1 == NULL) {
        printf("  FAILED: malloc returned NULL\n");
        return 1;
    }
    for (int i = 0; i < 10; i++) arr1[i] = i;
    printf("  PASSED\n");
    
    // Test 2: Calloc (zero-init)
    printf("Test 2: Calloc (zero-init)...\n");
    int* arr2 = (int*)calloc(10, sizeof(int));
    if (arr2[5] != 0) {
        printf("  FAILED: calloc didn't zero-init\n");
        return 1;
    }
    printf("  PASSED\n");
    
    // Test 3: Realloc
    printf("Test 3: Realloc...\n");
    int* arr3 = (int*)malloc(10 * sizeof(int));
    arr3 = (int*)realloc(arr3, 20 * sizeof(int));
    if (arr3 == NULL) {
        printf("  FAILED: realloc failed\n");
        return 1;
    }
    printf("  PASSED\n");
    
    // Test 4: Free and reuse
    printf("Test 4: Free and reuse...\n");
    free(arr1);
    arr1 = (int*)malloc(10 * sizeof(int));
    if (arr1 == NULL) {
        printf("  FAILED: reallocation after free failed\n");
        return 1;
    }
    printf("  PASSED\n");
    
    // Cleanup
    free(arr1);
    free(arr2);
    free(arr3);
    
    printf("=== All Tests Passed ===\n");
    return 0;
}
```

---

## References
- [OSDev: Paging](https://wiki.osdev.org/Paging)
- [OSDev: Physical Memory Manager](https://wiki.osdev.org/Physical_Memory_Manager)
- [Wikipedia: Memory Management Unit](https://en.wikipedia.org/wiki/Memory_management_unit)
- [Wikipedia: C Dynamic Memory Allocation](https://en.wikipedia.org/wiki/C_dynamic_memory_allocation)
- [Wikipedia: Paging](https://en.wikipedia.org/wiki/Paging)
