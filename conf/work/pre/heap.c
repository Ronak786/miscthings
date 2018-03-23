/*
 * Copyright (c) 2008-2009 Travis Geiselbrecht
 * Copyright (c) 2009 Corey Tabaka
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * NOTE: we disable every cirtical section check and assert in
 * this preloader version of heap function
 */
#include <string.h>

#include "list.h"
#include "heap.h"

#define DEBUG_HEAP 0
#define ALLOC_FILL 0x99
#define FREE_FILL 0x77
#define PADDING_FILL 0x55
#define PADDING_SIZE 64

#define HEAP_MAGIC 'HEAP'
#define ROUNDUP(a, b) (((a) + ((b)-1)) & ~((b)-1))

// should modify here to set the whole heap just here!!!!

#define HEAP_LEN 14*1024
static char my_local_heap[HEAP_LEN];
#define HEAP_START ((unsigned long)my_local_heap)

struct free_heap_chunk {
	struct list_node node;
	size_t len;
};

struct heap {
	void *base;
	size_t len;
	struct list_node free_list;
};

// heap static vars
static struct heap theheap;

// structure placed at the beginning every allocation
struct alloc_struct_begin {
	unsigned int magic;
	void *ptr;
	size_t size;
#if DEBUG_HEAP
	void *padding_start;
	size_t padding_size;
#endif
};


// try to insert this free chunk into the free list, consuming the chunk by merging it with
// nearby ones if possible. Returns base of whatever chunk it became in the list.
static struct free_heap_chunk *heap_insert_free_chunk(struct free_heap_chunk *chunk)
{
#if 0
//#if DEBUGLEVEL > INFO
	vaddr_t chunk_end = (vaddr_t)chunk + chunk->len;
#endif

	struct free_heap_chunk *next_chunk;
	struct free_heap_chunk *last_chunk;

	// walk through the list, finding the node to insert before
	list_for_every_entry(&theheap.free_list, next_chunk, struct free_heap_chunk, node) {
		if (chunk < next_chunk) {
//			assert(chunk_end <= (vaddr_t)next_chunk);

			list_add_before(&next_chunk->node, &chunk->node);

			goto try_merge;
		}
	}

	// walked off the end of the list, add it at the tail
	list_add_tail(&theheap.free_list, &chunk->node);

	// try to merge with the previous chunk
try_merge:
	last_chunk = list_prev_type(&theheap.free_list, &chunk->node, struct free_heap_chunk, node);
	if (last_chunk) {
		if ((vaddr_t)last_chunk + last_chunk->len == (vaddr_t)chunk) {
			// easy, just extend the previous chunk
			last_chunk->len += chunk->len;
			
			// remove ourself from the list
			list_delete(&chunk->node);
			
			// set the chunk pointer to the newly extended chunk, in case 
			// it needs to merge with the next chunk below
			chunk = last_chunk;
		}
	}

	// try to merge with the next chunk
	if (next_chunk) {
		if ((vaddr_t)chunk + chunk->len == (vaddr_t)next_chunk) {
			// extend our chunk
			chunk->len += next_chunk->len;

			// remove them from the list
			list_delete(&next_chunk->node);
		}
	}

	return chunk;
}

struct free_heap_chunk *heap_create_free_chunk(void *ptr, size_t len)
{
//	assert((len % sizeof(void *)) == 0); // size must be aligned on pointer boundary

#if DEBUG_HEAP
	memset(ptr, FREE_FILL, len);
#endif

	struct free_heap_chunk *chunk = (struct free_heap_chunk *)ptr;
	chunk->len = len;

	return chunk;
}

void *heap_alloc(size_t size, unsigned int alignment)
{
	void *ptr;
#if DEBUG_HEAP
	size_t original_size = size;
#endif
	
	//printf/g("size %zd, align %d\n", size, alignment);

	// alignment must be power of 2
	if (alignment & (alignment - 1))
		return NULL;

	// we always put a size field + base pointer + magic in front of the allocation
	size += sizeof(struct alloc_struct_begin);
#if DEBUG_HEAP
	size += PADDING_SIZE;
#endif

	// make sure we allocate at least the size of a struct free_heap_chunk so that
	// when we free it, we can create a struct free_heap_chunk struct and stick it
	// in the spot
	if (size < sizeof(struct free_heap_chunk))
		size = sizeof(struct free_heap_chunk);

	// round up size to a multiple of native pointer size
	size = ROUNDUP(size, sizeof(void *));

	// deal with nonzero alignments
	if (alignment > 0) {
		if (alignment < 16)
			alignment = 16;

		// add alignment for worst case fit
		size += alignment;
	}

	// critical section
//	enter_critical_section();

	// walk through the list
	ptr = NULL;
	struct free_heap_chunk *chunk;
	list_for_every_entry(&theheap.free_list, chunk, struct free_heap_chunk, node) {
//		assert((chunk->len % sizeof(void *)) == 0); // len should always be a multiple of pointer size

		// is it big enough to service our allocation?
		if (chunk->len >= size) {
			ptr = chunk;

			// remove it from the list
			struct list_node *next_node = list_next(&theheap.free_list, &chunk->node);
			list_delete(&chunk->node);

			if (chunk->len > size + sizeof(struct free_heap_chunk)) {
				// there's enough space in this chunk to create a new one after the allocation
				struct free_heap_chunk *newchunk = heap_create_free_chunk((uint8_t *)ptr + size, chunk->len - size);

				// truncate this chunk
				chunk->len -= chunk->len - size;

				// add the new one where chunk used to be
				if (next_node)
					list_add_before(next_node, &newchunk->node);
				else
					list_add_tail(&theheap.free_list, &newchunk->node);
			}

			// the allocated size is actually the length of this chunk, not the size requested
//			assert(chunk->len >= size);
			size = chunk->len;

#if DEBUG_HEAP
			memset(ptr, ALLOC_FILL, size);
#endif

			ptr = (void *)((addr_t)ptr + sizeof(struct alloc_struct_begin));

			// align the output if requested
			if (alignment > 0) {
				ptr = (void *)ROUNDUP((addr_t)ptr, alignment);
			}

			struct alloc_struct_begin *as = (struct alloc_struct_begin *)ptr;
			as--;
			as->magic = HEAP_MAGIC;
			as->ptr = (void *)chunk;
			as->size = size;
#if DEBUG_HEAP
			as->padding_start = ((uint8_t *)ptr + original_size);
			as->padding_size = (((addr_t)chunk + size) - ((addr_t)ptr + original_size));
//			//printf/g("padding start %p, size %u, chunk %p, size %u\n", as->padding_start, as->padding_size, chunk, size);

			memset(as->padding_start, PADDING_FILL, as->padding_size);
#endif

			break;
		}
	}

//	exit_critical_section();

	return ptr;
}

void *heap_realloc(void *ptr, size_t size)
{
	void * tmp_ptr = NULL;
	size_t min_size;
	struct alloc_struct_begin *as = (struct alloc_struct_begin *)ptr;
	as--;

	if (size != 0){
		tmp_ptr = heap_alloc(size, 0);
		if (ptr != NULL && tmp_ptr != NULL){
			min_size = (size < as->size) ? size : as->size;
			memcpy(tmp_ptr, ptr, min_size);
			heap_free(ptr);
		}
	} else {
		if (ptr != NULL)
			heap_free(ptr);
	}
	return(tmp_ptr);
}


void heap_free(void *ptr)
{
	if (ptr == 0)
		return;

	// check for the old allocation structure
	struct alloc_struct_begin *as = (struct alloc_struct_begin *)ptr;
	as--;
	
//	assert(as->magic == HEAP_MAGIC);

#if DEBUG_HEAP
	{
		uint i;
		uint8_t *pad = (uint8_t *)as->padding_start;

		for (i = 0; i < as->padding_size; i++) {
			if (pad[i] != PADDING_FILL) {
				//printf/g("free at %p scribbled outside the lines:\n", ptr);
				hexdump(pad, as->padding_size);
				panic("die\n");
			}
		}
	}
#endif

	// looks good, create a free chunk and add it to the pool
//	enter_critical_section();
	heap_insert_free_chunk(heap_create_free_chunk(as->ptr, as->size));
//	exit_critical_section();
}

void heap_init(void)
{

	// set the heap range
	theheap.base = (void *)HEAP_START;
	theheap.len = HEAP_LEN;

	// initialize the free list
	list_initialize(&theheap.free_list);

	// create an initial free chunk
	heap_insert_free_chunk(heap_create_free_chunk(theheap.base, theheap.len));
}

