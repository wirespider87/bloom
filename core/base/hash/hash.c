#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "core/base/hash/hash.h"

bloom_id bloom_hash_str(const char *str)
{
    return bloom_hash_str_seed(str, BLOOM_HASH_SEED);
}

bloom_id bloom_hash_str_seed(const char *str, bloom_id seed)
{
    bloom_id hash = seed;
    while (*str)
    {
        hash ^= (bloom_u8)*str++;
        hash *= 0x01000193u;
    }
    return hash;
}

bloom_id bloom_hash_bytes(const void *data, bloom_u64 len, bloom_id seed)
{
    const bloom_u8 *bytes = (const bloom_u8 *)data;
    bloom_id hash = seed;
    bloom_u64 i;
    for (i = 0; i < len; i++)
    {
        hash ^= bytes[i];
        hash *= 0x01000193u;
    }
    return hash;
}
