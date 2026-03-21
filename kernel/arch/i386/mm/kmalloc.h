#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>
#include <stdint.h>

/**
 * kmalloc_init - Initialize the kernel heap.
 * @initial_size: The initial size of the heap to pre-allocate.
 */
void kmalloc_init(size_t initial_size);

/**
 * kmalloc - Allocate a block of memory.
 * @size: The size of the block to allocate.
 *
 * Return: A pointer to the allocated block, or NULL if allocation fails.
 */
void* kmalloc(size_t size);

/**
 * kfree - Free a block of memory.
 * @ptr: The pointer to the block to free.
 */
void kfree(void* ptr);

/**
 * kcalloc - Allocate a block of memory and clear it to zero.
 * @nmemb: The number of elements.
 * @size: The size of each element.
 *
 * Return: A pointer to the allocated block, or NULL if allocation fails.
 */
void* kcalloc(size_t nmemb, size_t size);

/**
 * krealloc - Resize a block of memory.
 * @ptr: The pointer to the block to resize.
 * @size: The new size of the block.
 *
 * Return: A pointer to the resized block, or NULL if allocation fails.
 */
void* krealloc(void* ptr, size_t size);

#endif // KMALLOC_H
