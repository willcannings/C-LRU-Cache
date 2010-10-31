#include <pthread.h>
#include <stdint.h>

#ifndef __lru_cache__
#define __lru_cache__

typedef enum {
  NO_ERROR = 0,
  MISSING_CACHE,
  MISSING_KEY,
  MISSING_VALUE,
  PTHREAD_ERROR
} lru_cache_error;

typedef struct {
  void      *value;
  void      *key;
  uint32_t  value_length;
  uint32_t  key_length;
  uint64_t  access_count;
  struct lru_cache_item *next;
} lru_cache_item;

typedef struct {
  lru_cache_item  **items;
  uint64_t        access_count;
  uint64_t        free_memory;
  uint32_t        hash_table_size;
  pthread_mutex_t *mutex;
} lru_cache;

#endif
