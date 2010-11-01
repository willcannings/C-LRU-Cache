#include <pthread.h>
#include <stdint.h>
#include <time.h>

#ifndef __lru_cache__
#define __lru_cache__

// ------------------------------------------
// errors
// ------------------------------------------
typedef enum {
  LRU_CACHE_NO_ERROR = 0,
  LRU_CACHE_MISSING_CACHE,
  LRU_CACHE_MISSING_KEY,
  LRU_CACHE_MISSING_VALUE,
  LRU_CACHE_PTHREAD_ERROR,
  LRU_CACHE_VALUE_TOO_LARGE
} lru_cache_error;


// ------------------------------------------
// types
// ------------------------------------------
typedef struct {
  void      *value;
  void      *key;
  uint32_t  value_length;
  uint32_t  key_length;
  uint64_t  access_count;
  void      *next;
} lru_cache_item;

typedef struct {
  lru_cache_item  **items;
  uint64_t        access_count;
  uint64_t        free_memory;
  uint64_t        total_memory;
  uint64_t        average_item_length;
  uint32_t        hash_table_size;
  time_t          seed;
  lru_cache_item  *free_items;
  pthread_mutex_t *mutex;
} lru_cache;


// ------------------------------------------
// api
// ------------------------------------------
lru_cache *lru_cache_new(uint64_t cache_size, uint32_t average_length);
lru_cache_error lru_cache_free(lru_cache *cache);
lru_cache_error lru_cache_set(lru_cache *cache, void *key, uint32_t key_length, void *value, uint32_t value_length);
lru_cache_error lru_cache_get(lru_cache *cache, void *key, uint32_t key_length, void **value);
lru_cache_error lru_cache_delete(lru_cache *cache, void *key, uint32_t key_length);

#endif
