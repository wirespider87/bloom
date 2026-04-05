#ifndef BLOOM_CORE_BASE_HASH_H
#define BLOOM_CORE_BASE_HASH_H

#include "core/base/types/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(BLOOM_INTERNAL_BUILD) || defined(BLOOM_ENABLE_LEGACY_API)
bloom_id bloom_hash_str(const char *str);
bloom_id bloom_hash_str_seed(const char *str, bloom_id seed);
bloom_id bloom_hash_bytes(const void *data, bloom_u64 len, bloom_id seed);
#endif

#ifdef __cplusplus
}
#endif

#endif
