#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "core/base/memory/memory.h"
#include <string.h>

void bloom_arena_init(bloom_arena *a, void *buffer, bloom_u64 size)
{
    a->base = (bloom_u8 *)buffer;
    a->size = size;
    a->used = 0;
}

void *bloom_arena_alloc(bloom_arena *a, bloom_u64 size)
{
    bloom_u64 aligned = (size + 7) & ~(bloom_u64)7;
    if (a->used + aligned > a->size)
    {
        return NULL;
    }
    void *ptr = a->base + a->used;
    a->used += aligned;
    memset(ptr, 0, aligned);
    return ptr;
}

void bloom_arena_reset(bloom_arena *a)
{
    a->used = 0;
}

bloom_u64 bloom_arena_remaining(bloom_arena *a)
{
    return a->size - a->used;
}

void bloom_pool_init(bloom_pool *p, void *buffer, bloom_u64 chunk_size, bloom_u64 count)
{
    bloom_u64 aligned_size = (chunk_size + 7) & ~(bloom_u64)7;
    if (aligned_size < sizeof(bloom_pool_chunk))
    {
        aligned_size = sizeof(bloom_pool_chunk);
    }
    p->base = (bloom_u8 *)buffer;
    p->chunk_size = aligned_size;
    p->count = count;
    p->free_list = NULL;

    bloom_u64 i;
    for (i = 0; i < count; i++)
    {
        bloom_pool_chunk *chunk = (bloom_pool_chunk *)(p->base + i * aligned_size);
        chunk->next = p->free_list;
        p->free_list = chunk;
    }
}

void *bloom_pool_alloc(bloom_pool *p)
{
    if (!p->free_list)
    {
        return NULL;
    }
    bloom_pool_chunk *chunk = p->free_list;
    p->free_list = chunk->next;
    memset(chunk, 0, p->chunk_size);
    return chunk;
}

void bloom_pool_free(bloom_pool *p, void *ptr)
{
    if (!ptr)
    {
        return;
    }
    bloom_pool_chunk *chunk = (bloom_pool_chunk *)ptr;
    chunk->next = p->free_list;
    p->free_list = chunk;
}
