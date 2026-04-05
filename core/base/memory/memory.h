#ifndef BLOOM_CORE_BASE_MEMORY_H
#define BLOOM_CORE_BASE_MEMORY_H

#include "core/base/types/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bloom_arena
{
    bloom_u8 *base;
    bloom_u64 size;
    bloom_u64 used;
} bloom_arena;

typedef struct bloom_pool_chunk
{
    struct bloom_pool_chunk *next;
} bloom_pool_chunk;

typedef struct bloom_pool
{
    bloom_u8 *base;
    bloom_u64 chunk_size;
    bloom_u64 count;
    bloom_pool_chunk *free_list;
} bloom_pool;

#if defined(BLOOM_INTERNAL_BUILD) || defined(BLOOM_ENABLE_LEGACY_API)
void  bloom_arena_init(bloom_arena *a, void *buffer, bloom_u64 size);
void *bloom_arena_alloc(bloom_arena *a, bloom_u64 size);
void  bloom_arena_reset(bloom_arena *a);
bloom_u64 bloom_arena_remaining(bloom_arena *a);

void  bloom_pool_init(bloom_pool *p, void *buffer, bloom_u64 chunk_size, bloom_u64 count);
void *bloom_pool_alloc(bloom_pool *p);
void  bloom_pool_free(bloom_pool *p, void *ptr);
#endif

#ifdef __cplusplus
}
#endif

#endif
