# Memory Management Internals

TilekarOS uses a tiered memory management system, transitioning from raw physical memory allocation to virtual memory with recursive paging.

## 1. Physical Memory Manager (PMM)

The PMM tracks 4KB page frames in physical RAM using a **bitmap**.

- **Allocation**: `pmm_alloc_page_frame()` finds the first free bit and returns the physical address.
- **Deallocation**: `pmm_free_page_frame(phys_addr)` clears the bit.

---

## 2. Virtual Memory Manager (VMM) & Paging

TilekarOS implements **32-bit Paging** with a **Flat Memory Model**.

### Memory Map
- **0x00000000 - 0xBFFFFFFF**: User Space (3GB)
- **0xC0000000 - 0xFFFFFFFF**: Kernel Space (1GB, Higher-Half)

### Recursive Paging
To modify page directories/tables while paging is enabled, we map the Page Directory into itself at the last entry (1023).

- **PD access**: 0xFFFFF000
- **PT access**: 0xFFC00000 - 0xFFFFEFFF

---

## 3. Kernel Heap (kmalloc)

TilekarOS provides a standard allocation interface using a **doubly linked list** of blocks.

### Initialization (`kmalloc_init`)
The heap is initialized during kernel startup with a call to `kmalloc_init(initial_size)`. It sets the base of the heap at the `KERNEL_MALLOC` virtual address (defined in `local_config.h`).

### Allocation Strategy
- **First-Fit**: The allocator searches from the `head` for the first free block large enough for the request.
- **8-Byte Alignment**: All allocations are automatically aligned to 8-byte boundaries to satisfy CPU requirements and improve performance.
- **Block Splitting**: If a block is significantly larger than requested, it is split into two to reduce internal fragmentation.
- **Heap Extension (`heap_sbrk`)**: If no suitable block is found, `heap_sbrk` is called. This function:
    1. Calculates how many new 4KB physical pages are needed.
    2. Allocates them via the PMM.
    3. Maps them into the virtual address space using the VMM.
    4. Extends the linked list of blocks.

### Deallocation (`kfree`)
- **Coalescing**: When a block is freed, the allocator automatically merges it with any adjacent free blocks (previous or next) to combat external fragmentation.

### Thread Safety
Both `kmalloc` and `kfree` are wrapped in `interrupt_save()` and `interrupt_restore()` calls. This ensures that memory allocation is atomic and safe even if an interrupt occurs (e.g., a timer interrupt triggering a context switch) during the process.

### API
- `kmalloc(size)`: Allocates `size` bytes. Returns a pointer to the payload.
- `kfree(ptr)`: Frees the block starting at `ptr`.
- `kcalloc(n, size)`: Allocates `n * size` bytes and clears the memory to zero.
- `krealloc(ptr, size)`: Resizes an allocation. If the new size is smaller, it may return the same pointer. If larger, it allocates a new block and copies the data.
