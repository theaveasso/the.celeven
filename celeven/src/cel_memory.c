#include <assert.h>
#include <string.h>

#include "cel.h"

Internal uintptr_t align_forward(uintptr_t ptr, size_t align) {
    uintptr_t p, a, mod;
    assert(is_power_of_two(align));

    p   = ptr;
    a   = (uintptr_t) align;
    mod = p & (a - 1);// same as (p%a) but faster as 'a' is a power of two

    if (mod != 0) { p += a - mod; }// if 'p' addr is not aligned, push the addr to the next value which is aligned
    return p;
}

Internal size_t calc_padding_with_header(uintptr_t ptr, uintptr_t alignment, size_t header_size) {
    assert(is_power_of_two(alignment));
    size_t padding;
    uintptr_t aligned_addr = align_forward(ptr + header_size, alignment);
    padding                = aligned_addr - ptr;
    return padding;
}

void arena_init(CELarena *a, void *backing_buffer, size_t backing_buffer_len) {
    a->buf         = (unsigned char *) backing_buffer;
    a->buf_len     = backing_buffer_len;
    a->prev_offset = 0;
    a->curr_offset = 0;
}

void *arena_alloc(CELarena *a, size_t len) {
    return arena_alloc_align(a, len, DEFAULT_ALIGNMENT);
}

void *arena_alloc_align(CELarena *a, size_t len, size_t align) {
    // align 'curr_offset' forward to the specified alignment
    uintptr_t curr_ptr = (uintptr_t) a->buf + (uintptr_t) a->curr_offset;
    uintptr_t offset   = align_forward(curr_ptr, align);
    offset -= (uintptr_t) a->buf;

    // check to see if the backing memory has space left
    if (offset + len <= a->buf_len)
    {
        void *ptr      = &a->buf[offset];
        a->prev_offset = offset;
        a->curr_offset = offset + len;

        // zero new memory by default
        memset(ptr, 0, len);
        return ptr;
    }

    // return 'NULL' if the arena is out of memory (or handle differently)
    return NULL;
}

void *arena_resize(CELarena *a, void *oldmem, size_t osize, size_t nsize) {
    return arena_resize_align(a, oldmem, osize, nsize, DEFAULT_ALIGNMENT);
}

void *arena_resize_align(CELarena *a, void *oldmem, size_t osize, size_t nsize, size_t align) {
    unsigned char *old_mem = (unsigned char *) oldmem;
    assert(is_power_of_two(align));

    if (old_mem == NULL || osize == 0)
    {
        return arena_alloc(a, nsize);
    }
    else if (a->buf <= old_mem && old_mem < a->buf + a->buf_len)
    {
        if (a->buf + a->prev_offset == old_mem)
        {
            a->curr_offset = a->prev_offset + nsize;
            if (nsize > osize) { memset(&a->buf[a->curr_offset], 0, nsize - osize); }
            return old_mem;
        }
    }

    void *new_mem    = arena_alloc(a, nsize);
    size_t copy_size = osize < nsize ? osize : nsize;
    memmove(new_mem, oldmem, copy_size);
    return new_mem;
}

void arena_free(CELarena *a, void *ptr) {
    (void) a;
    (void) ptr;
}

void arena_free_all(CELarena *a) {
    a->prev_offset = 0;
    a->curr_offset = 0;
}

void arena_debug_print(CELarena *a, const char *label) {
    printf("[Arena: %s]\n", label);
    printf("- Buffer Address:      %p\n", a->buf);
    printf("- Buffer Size:         %zu bytes\n", a->buf_len);
    printf("- Previous Offset:     %zu\n", a->prev_offset);
    printf("- Current Offset:      %zu\n", a->curr_offset);
    printf("- Used Memory:         %zu bytes\n", a->curr_offset);
    printf("- Remaining Memory:    %zu bytes\n\n", a->buf_len - a->curr_offset);
}

void stack_init(CELstack *s, void *backing_buffer, size_t backing_buffer_len) {
    s->buf         = (unsigned char *) backing_buffer;
    s->buf_len     = backing_buffer_len;
    s->prev_offset = 0;
    s->curr_offset = 0;
}

void *stack_alloc(CELstack *s, size_t len) {
    return stack_alloc_align(s, len, DEFAULT_ALIGNMENT);
}

void *stack_alloc_align(CELstack *s, size_t len, size_t align) {
    uintptr_t curr_addr, next_addr;
    size_t padding;
    CELstack_header *header;

    assert(is_power_of_two(align));

    // as the padding is 8 bits (1 byte), the largest alignment that can be used is 128 bytes
    if (align > 128) { align = 128; }

    curr_addr = (uintptr_t) s->buf + (uintptr_t) s->curr_offset;
    padding   = calc_padding_with_header(curr_addr, (uintptr_t) align, sizeof(CELstack_header));

    // stack allocator is out of memory
    if (s->curr_offset + padding + len > s->buf_len) { return NULL; }

    s->prev_offset = s->curr_offset;
    s->curr_offset += padding;
    next_addr           = curr_addr + (uintptr_t) padding;
    header              = (CELstack_header *) (next_addr - sizeof(CELstack_header));
    header->padding     = (uint8_t) padding;
    header->prev_offset = s->prev_offset;

    s->curr_offset += len;
    return memset((void *) next_addr, 0, len);
}

void *stack_resize(CELstack *s, void *oldmem, size_t osize, size_t nsize) {
    return stack_resize_align(s, oldmem, osize, nsize, DEFAULT_ALIGNMENT);
}

void *stack_resize_align(CELstack *s, void *oldmem, size_t osize, size_t nsize, size_t align) {
    if (oldmem == NULL)
    {
        return stack_alloc_align(s, nsize, align);
    }
    else if (nsize == 0)
    {
        stack_free(s, oldmem);
        return NULL;
    }

    size_t min_size = osize < nsize ? osize : nsize;

    uintptr_t start     = (uintptr_t) s->buf;
    uintptr_t end       = start + (uintptr_t) s->buf_len;
    uintptr_t curr_addr = (uintptr_t) oldmem;

    if (!(start <= curr_addr && curr_addr < end))
    {
        assert(0 && "out of bounds memory address passed to stack allocator (resize)");
        return NULL;
    }

    // check if old memory is the top-most allocation
    uintptr_t expected_top = (uintptr_t) s->buf + s->curr_offset - osize;
    if (curr_addr != expected_top)
    {
        void *new_ptr = stack_alloc_align(s, nsize, align);
        if (new_ptr)
        {
            memmove(new_ptr, oldmem, min_size);
        }
        return new_ptr;
    }

    // in place resize
    if (s->curr_offset - osize + nsize > s->buf_len) { return NULL; }// not enough space

    s->curr_offset = s->curr_offset - osize + nsize;
    return oldmem;
}

void stack_free(CELstack *s, void *ptr) {
    if (ptr != NULL)
    {
        uintptr_t start     = (uintptr_t) s->buf;
        uintptr_t end       = start + s->buf_len;
        uintptr_t curr_addr = (uintptr_t) ptr;

        if (!(start <= curr_addr && curr_addr < end))
        {
            assert(0 && "out of bounds memory address passed to stack allocator (free)");
            return;
        }

        CELstack_header *header = (CELstack_header *) ((uintptr_t) ptr - sizeof(CELstack_header));

        s->curr_offset = header->prev_offset;// restore allocator state to previous offset
        s->prev_offset = 0;
    }
}

void stack_free_all(CELstack *s) {
    s->prev_offset = 0;
    s->curr_offset = 0;
}

void stack_debug_print(CELstack *s, const char *label) {
}

void pool_init(CELpool *p, void *backing_buffer, size_t backing_buffer_len, size_t chunk_size, size_t chunk_alignment) {
    uintptr_t initial_start = (uintptr_t) backing_buffer;
    uintptr_t start         = align_forward(initial_start, (uintptr_t) chunk_alignment);
    backing_buffer_len -= (size_t) (start - initial_start);

    chunk_size = align_forward(chunk_size, chunk_alignment);
    assert(chunk_size >= sizeof(CELpool_free_node) && "chunk size is to small");
    assert(backing_buffer_len >= chunk_size && "backing buffer len is smaller than the chunk size");

    p->buf        = (unsigned char *) backing_buffer;
    p->buf_len    = backing_buffer_len;
    p->chunk_size = chunk_size;
    p->head       = NULL;
}

void *pool_alloc(CELpool *p, size_t len) {
    CELpool_free_node *node = p->head;
    if (node == NULL)
    {
        assert(false && "pool allocator has no free memory");
        return NULL;
    }

    p->head = p->head->next;// pop free node
    return memset(node, 0, p->chunk_size);
}

void *pool_alloc_align(CELpool *p, size_t len, size_t align) {
    return NULL;
}

void *pool_resize(CELpool *p, void *oldmem, size_t osize, size_t nsize) {
    return NULL;
}

void *pool_resize_align(CELpool *p, void *oldmem, size_t osize, size_t nsize, size_t align) {
    return NULL;
}

void pool_free(CELpool *p, void *ptr) {
    CELpool_free_node *node;

    void *start = p->buf;
    void *end   = &p->buf[p->buf_len];

    if (ptr == NULL) { return; }
    if (!(start <= ptr && ptr < end))
    {
        assert(false && "memory is out of bounds of the buffer in this pool (free)");
        return;
    }

    node       = (CELpool_free_node *) ptr;
    node->next = p->head;
    p->head    = node;
}

void pool_free_all(CELpool *p) {
    size_t chunk_count = p->buf_len / p->chunk_size;
    size_t i;

    for (i = 0; i < chunk_count; ++i)
    {
        pool_free(p, &p->buf[i * p->chunk_size]);
    }
}

void pool_debug_print(CELpool *p, const char *label) {
}
