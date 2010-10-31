#include "lru_cache.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// bernstein hash function
lru_cache_item *lru_cache_key_to_item(lru_cache *cache, void *key, uint32_t length) {
  uint32_t hash = 5381, i = 0;
  for(; i < length; i++)
    hash = ((hash << 5) + hash + ((char *)key)[i]);
  return cache->items[hash % cache->hash_table_size];
}

lru_cache *lru_cache_new(uint64_t max_size, uint32_t average_size) {
  // create the cache and size the hash table to reasonably fit the expected number of items
  lru_cache *cache        = (lru_cache *) calloc(sizeof(lru_cache), 1);
  cache->free_memory      = max_size;
  cache->hash_table_size  = max_size / average_size;
  cache->items            = (lru_cache_item **) calloc(sizeof(lru_cache_item *), cache->hash_table_size);
  
  // a mutex is used so the cache can be thread safe
  cache->mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
  if(pthread_mutex_init(cache->mutex, NULL)) {
    perror("LRU Cache unable to initialise mutex");
    if(cache) {
      if(cache->items)
        free(cache->items);
      free(cache);
    }
    return NULL;
  }
  
  return cache;
}

void lru_cache_free(lru_cache *cache) {
  // free each of the cached items
  lru_cache_item *item = NULL, *next = NULL;
  uint32_t i = 0;
  for(; i < cache->hash_table_size; i++) {
    item = cache->items[i];    
    while(item != NULL) {
      next = item->next;
      free(item);
      item = next;
    }
  }
  
  // free the cache itself
  free(cache->items);
  free(cache);
}

int lru_cache_set(lru_cache *cache, void *key, uint32_t key_length, void *item, uint32_t item_length) {
  
}

int lru_cache_get(lru_cache *cache, void *key, uint32_t key_length, void **item) {
  lru_cache_item *item = lru_cache_key_to_item(cache, key, key_length);
}

int lru_cache_delete(lru_cache *cache, void *key, uint32_t key_length) {
  
}
