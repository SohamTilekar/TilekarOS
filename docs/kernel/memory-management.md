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
    --8<-- "kernel/arch/i386/mm/memory.c"
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
    --8<-- "kernel/arch/i386/mm/kmalloc.c"
    ```

### Deallocation (`kfree`):
- **Coalescing**: When a block is freed, it is merged with its neighbors (if they are also free) to reduce fragmentation.

---

## 4. Test/Example: Stress-Testing the Heap

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

---

## References
- [OSDev: Paging](https://wiki.osdev.org/Paging)
- [OSDev: Physical Memory Manager](https://wiki.osdev.org/Physical_Memory_Manager)
- [Wikipedia: Memory Management Unit](https://en.wikipedia.org/wiki/Memory_management_unit)
- [Wikipedia: C Dynamic Memory Allocation](https://en.wikipedia.org/wiki/C_dynamic_memory_allocation)
- [Wikipedia: Paging](https://en.wikipedia.org/wiki/Paging)
