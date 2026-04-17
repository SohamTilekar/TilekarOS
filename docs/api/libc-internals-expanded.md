# LibC Internals (Expanded)

This document summarizes the implementation details of the libc allocators and stdio internals.

1) User-Space malloc (libc/stdlib/malloc.c)

- Allocation strategy: First-fit linear list of blocks stored in the heap space provided by sbrk()/brk().
- Block header (block_header_t):
  - uint32_t size; // payload size
  - uint32_t free; // 1 if free, 0 if allocated
- Alignment: 8-byte alignment via align_up.
- Splitting: A free block is split if remaining space after allocation is larger than sizeof(header) + MALLOC_SPLIT_MIN (8 bytes).
- Coalescing: free() walks the heap and coalesces adjacent free blocks; also shrinks the program break if the last block is free.
- brk()/sbrk(): Implemented via SYS_BRK syscall wrapper (__syscall(SYS_BRK,...)). sbrk(0) returns current break.
- Realloc: Allocates new block and copies old data if needed.

2) Kernel kmalloc (kernel/arch/i386/mm/kmalloc.c)

- Kernel heap maintained as a doubly-linked list of block_header_t with fields: size, is_free, is_fake, next, prev.
- kmalloc_init(initial_size) sets up initial region by calling heap_sbrk.
- Heap extension: heap_sbrk increments heap_end and maps new physical pages using pmm_alloc_page_frame() and memory_map_page(). Uses interrupt_save/restore to be atomic.
- Allocation: First-fit scan; if found and large enough, splits block. If last free block can be extended, calls heap_sbrk for additional space. Otherwise requests new block via heap_sbrk.
- Aligned allocations: kmalloc_aligned allocates extra space, computes aligned address, and uses a "fake" header before aligned address with is_fake flag and prev pointing to real header.
- Freeing: kfree marks block free and coalesces with adjacent free blocks. Handles fake headers by resolving to real header.
- kcalloc and krealloc implemented atop kmalloc/kfree with memset/memcpy.

3) stdio notes
- FILE struct is minimal (int fd) and stdio functions (printf, puts, putchar, fopen) are provided; see libc/stdio for implementations.
- printf family uses vsnprintf and vsprintf implementations in stdio directory.

4) Thread-safety and atomicity
- Kernel allocator uses interrupt_save/restore to prevent concurrent modifications during allocation/free.
- User-space allocator is single-thread/process scoped and relies on syscalls for brk; no locks implemented in libc malloc for multithreaded use.

5) Errors and edge cases
- Both allocators return NULL on OOM.
- kmalloc handles alignment edge-cases with fake headers; krealloc copies appropriate amount when resizing.

References:
- kernel/arch/i386/mm/kmalloc.c
- libc/stdlib/malloc.c
- libc/include/stdlib.h
