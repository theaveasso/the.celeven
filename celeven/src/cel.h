#pragma once

#include "cel_define.h"

#include <stdint.h>
#include <stdio.h>

typedef struct CELarena CELarena;
struct CELarena {
    unsigned char *buf;
    size_t buf_len;
    size_t prev_offset;
    size_t curr_offset;
};

typedef struct CELstack_header CELstack_header;
struct CELstack_header {
    size_t padding;
    size_t prev_offset;
};

typedef struct CELstack CELstack;
struct CELstack {
    unsigned char *buf;
    size_t buf_len;
    size_t prev_offset;
    size_t curr_offset;
};

typedef struct CELpool_free_node CELpool_free_node;
struct CELpool_free_node {
    CELpool_free_node *next;
};

typedef struct CELpool CELpool;
struct CELpool {
    unsigned char *buf;
    size_t buf_len;
    size_t chunk_size;
    CELpool_free_node *head;
};

typedef struct CELmemory CELmemory;
struct CELmemory {
    CELarena transient_arena;
    CELarena persistent_arena;
};

typedef struct CELconfig CELconfig;
struct CELconfig {
    uint16_t width;
    uint16_t height;
    const char *title;
};

typedef struct CELgame CELgame;

typedef bool (*CELgame_init_fn)(struct CELgame *game_inst);
typedef bool (*CELgame_update_fn)(struct CELgame *game_inst);
typedef bool (*CELgame_draw_fn)(struct CELgame *game_inst);
typedef bool (*CELgame_destroy_fn)(struct CELgame *game_inst);

struct CELgame {
    CELconfig config;
    CELgame_init_fn game_init;
    CELgame_update_fn game_update;
    CELgame_draw_fn game_draw;
    CELgame_destroy_fn game_destroy;

    CELmemory state;
};

CELAPI inline bool is_power_of_two(uintptr_t x) { return (x & (x - 1)) == 0; }

CELAPI void arena_init(CELarena *a, void *backing_buffer, size_t backing_buffer_len);
CELAPI void *arena_alloc(CELarena *a, size_t len);
CELAPI void *arena_alloc_align(CELarena *a, size_t len, size_t align);
CELAPI void *arena_resize(CELarena *a, void *oldmem, size_t osize, size_t nsize);
CELAPI void *arena_resize_align(CELarena *a, void *oldmem, size_t osize, size_t nsize, size_t align);
CELAPI void arena_free(CELarena *a, void *ptr);
CELAPI void arena_free_all(CELarena *a);
CELAPI void arena_debug_print(CELarena *a, const char *label);

#define cel_arena_init(a, backing_buffer, backing_buffer_len) arena_init(a, backing_buffer, backing_buffer_len)
#define cel_arena_alloc(a, len) arena_alloc(a, len)
#define cel_arena_resize(a, old_mem, old_size, new_size) arena_resize(a, old_mem, old_size, new_size)
#define cel_arena_free(a, ptr) arena_free(a, ptr)
#define cel_arena_free_all(a) arena_free_all(a)
#define cel_arena_dbg_print(a, label) arena_debug_print(a, label);

CELAPI void stack_init(CELstack *s, void *backing_buffer, size_t backing_buffer_len);
CELAPI void *stack_alloc(CELstack *s, size_t len);
CELAPI void *stack_alloc_align(CELstack *s, size_t len, size_t align);
CELAPI void *stack_resize(CELstack *s, void *oldmem, size_t osize, size_t nsize);
CELAPI void *stack_resize_align(CELstack *s, void *oldmem, size_t osize, size_t nsize, size_t align);
CELAPI void stack_free(CELstack *s, void *ptr);
CELAPI void stack_free_all(CELstack *s);
CELAPI void stack_debug_print(CELstack *s, const char *label);

#define cel_stack_init(a, backing_buffer, backing_buffer_len) stack_init(a, backing_buffer, backing_buffer_len)
#define cel_stack_alloc(a, len) stack_alloc(a, len)
#define cel_stack_resize(a, old_mem, old_size, new_size) stack_resize(a, old_mem, old_size, new_size)
#define cel_stack_free(a, ptr) stack_free(a, ptr)
#define cel_stack_free_all(a) stack_free_all(a)
#define cel_stack_dbg_print(a, label) stack_debug_print(a, label);

CELAPI void pool_init(CELpool *p, void *backing_buffer, size_t backing_buffer_len, size_t chunk_size, size_t chunk_alignment);
CELAPI void *pool_alloc(CELpool *p, size_t len);
CELAPI void *pool_alloc_align(CELpool *p, size_t len, size_t align);
CELAPI void *pool_resize(CELpool *p, void *oldmem, size_t osize, size_t nsize);
CELAPI void *pool_resize_align(CELpool *p, void *oldmem, size_t osize, size_t nsize, size_t align);
CELAPI void pool_free(CELpool *p, void *ptr);
CELAPI void pool_free_all(CELpool *p);
CELAPI void pool_debug_print(CELpool *p, const char *label);

CELAPI bool application_init(CELgame *game);
CELAPI bool application_run();
