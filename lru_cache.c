#include "lru_cache.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <err.h>

// ------------------------------------------
// private functions
// ------------------------------------------
// bernstein hash function
lru_cache_item *lru_cache_key_to_item(lru_cache *cache, void *key, uint32_t length) {
  uint32_t hash = 5381, i = 0;
  for(; i < length; i++)
    hash = ((hash << 5) + hash + ((char *)key)[i]);
  return cache->items[hash % cache->hash_table_size];
}

// compare a key against an existing item's key
int lru_cache_cmp_keys(lru_cache_item *item, void *key, uint32_t key_length) {
  if(key_length != item->key_length) {
    return 1;
  } else {
    return memcmp(key, item->key, key_length);
  }
}


// ------------------------------------------
// public api
// ------------------------------------------
lru_cache *lru_cache_new(uint64_t max_size, uint32_t average_size) {
  // create the cache
  lru_cache *cache = (lru_cache *) calloc(sizeof(lru_cache), 1);
  if(!cache) {
    perror("LRU Cache unable to create cache object");
    return NULL;
  }
  cache->free_memory = max_size;
  cache->hash_table_size = max_size / average_size;
  
  // size the hash table to a guestimate of the number of slots required (assuming a perfect hash)
  cache->items = (lru_cache_item **) calloc(sizeof(lru_cache_item *), cache->hash_table_size);
  if(!cache->items) {
    perror("LRU Cache unable to create cache hash table");
    free(cache);
    return NULL;
  }
  
  // a mutex is used so the cache can be thread safe
  cache->mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
  if(pthread_mutex_init(cache->mutex, NULL)) {
    perror("LRU Cache unable to initialise mutex");
    free(cache->items);
    free(cache);
    return NULL;
  }
  return cache;
}


void lru_cache_free(lru_cache *cache) {
  // ensure we have a cache to free
  if(!cache) {
    return;
  }
  
  // free each of the cached items, and the hash table
  lru_cache_item *item = NULL, *next = NULL;
  uint32_t i = 0;
  if(cache->items) {
    for(; i < cache->hash_table_size; i++) {
      item = cache->items[i];    
      while(item) {
        next = item->next;
        free(item);
        item = next;
      }
    }
    free(cache->items);
  }
  
  // free the cache
  if(cache->mutex) {
    if(pthread_mutex_destroy(cache->mutex)) {
      perror("LRU Cache unable to destroy mutex");
    }
  }
  free(cache);
}


lru_cache_error lru_cache_set(lru_cache *cache, void *key, uint32_t key_length, void *value, uint32_t value_length) {
  
}


lru_cache_error lru_cache_get(lru_cache *cache, void *key, uint32_t key_length, void **value) {
  // preconditions
  if(!cache)
    return MISSING_CACHE;
  if(!key || key_length == 0)
    return MISSING_KEY;

  // lock access to the cache
  if(pthread_mutex_lock(cache->mutex)) {
    perror("LRU Cache unable to obtain mutex lock");
    return PTHREAD_ERROR;
  }
  
  // loop until we find the item, or hit the end of the chain
  lru_cache_item *item = lru_cache_key_to_item(cache, key, key_length);
  while(item && !lru_cache_cmp_keys(item, key, key_length)) {
    item = item->next;
  }
  
  // if we found the item, update the access counters
  if(item) {
    *value = item->value;
    item->access_count = ++cache->access_count;
  } else {
    *value = NULL;
  }
  
  // release access to the cache
  if(pthread_mutex_unlock(cache->mutex)) {
    perror("LRU Cache unable to release mutex lock");
    return PTHREAD_ERROR;
  }
  return NO_ERROR;
}


lru_cache_error lru_cache_delete(lru_cache *cache, void *key, uint32_t key_length) {
  
}
