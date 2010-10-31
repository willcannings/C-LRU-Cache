#include "lru_cache.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <err.h>

// ------------------------------------------
// private functions
// ------------------------------------------
// MurmurHash2, by Austin Appleby
// http://sites.google.com/site/murmurhash/
uint32_t lru_cache_hash(lru_cache *cache, void *key, uint32_t key_length) {
	uint32_t m = 0x5bd1e995;
	uint32_t r = 24;
	uint32_t h = cache->access_count ^ key_length;
	char* data = (char *)key;

	while(key_length >= 4) {
    uint32_t k = *(uint32_t *)data;
		k *= m;
		k ^= k >> r;
		k *= m;
		h *= m;
		h ^= k;
		data += 4;
		key_length -= 4;
	}
	
	switch(key_length) {
  	case 3: h ^= data[2] << 16;
  	case 2: h ^= data[1] << 8;
  	case 1: h ^= data[0];
  	        h *= m;
	};

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;
	return h % cache->hash_table_size;
}

// compare a key against an existing item's key
int lru_cache_cmp_keys(lru_cache_item *item, void *key, uint32_t key_length) {
  if(key_length != item->key_length)
    return 1;
  else
    return memcmp(key, item->key, key_length);
}

// remove the least recently used item
// TODO: we can optimise this by finding the n lru items, where n = required_space / average_length
void lru_cache_remove_lru_item(lru_cache *cache) {
  uint32_t i = 0, index = -1;
  uint64_t access_count = -1;
  
  for(; i < cache->hash_table_size; i++) {
    
  }
}

// remove an item and push it to the free items queue
void lru_cache_remove_item(lru_cache *cache, lru_cache_item *prev, lru_cache_item *item, uint32_t hash_index) {
  if(prev)
    prev->next = item->next;
  else
    cache->items[hash_index] = (lru_cache_item *) item->next;
  
  // push the item to the free items queue
  memset(item, 0, sizeof(lru_cache_item));
  item->next = (struct lru_cache_item *) cache->free_items;
  cache->free_items = item;
}

// error helpers
#define error_for(conditions, error)  if(conditions) {return error;}
#define test_for_missing_cache()      error_for(!cache, LRU_CACHE_MISSING_CACHE)
#define test_for_missing_key()        error_for(!key || key_length == 0, LRU_CACHE_MISSING_KEY)
#define test_for_missing_value()      error_for(!value || value_length == 0, LRU_CACHE_MISSING_VALUE)
#define test_for_value_too_large()    error_for(value_length > cache->total_memory, LRU_CACHE_VALUE_TOO_LARGE)

// lock helpers
#define lock_cache()    if(pthread_mutex_lock(cache->mutex)) {\
  perror("LRU Cache unable to obtain mutex lock");\
  return LRU_CACHE_PTHREAD_ERROR;\
}

#define unlock_cache()  if(pthread_mutex_unlock(cache->mutex)) {\
  perror("LRU Cache unable to release mutex lock");\
  return LRU_CACHE_PTHREAD_ERROR;\
}


// ------------------------------------------
// public api
// ------------------------------------------
lru_cache *lru_cache_new(uint64_t cache_size, uint32_t average_length) {
  // create the cache
  lru_cache *cache = (lru_cache *) calloc(sizeof(lru_cache), 1);
  if(!cache) {
    perror("LRU Cache unable to create cache object");
    return NULL;
  }
  cache->hash_table_size      = cache_size / average_length;
  cache->average_item_length  = average_length;
  cache->free_memory          = cache_size;
  cache->total_memory         = cache_size;
  
  // size the hash table to a guestimate of the number of slots required (assuming a perfect hash)
  cache->items = (lru_cache_item **) calloc(sizeof(lru_cache_item *), cache->hash_table_size);
  if(!cache->items) {
    perror("LRU Cache unable to create cache hash table");
    free(cache);
    return NULL;
  }
  
  // all cache calls are guarded by a mutex
  cache->mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
  if(pthread_mutex_init(cache->mutex, NULL)) {
    perror("LRU Cache unable to initialise mutex");
    free(cache->items);
    free(cache);
    return NULL;
  }
  return cache;
}


lru_cache_error lru_cache_free(lru_cache *cache) {
  test_for_missing_cache();
  
  // free each of the cached items, and the hash table
  lru_cache_item *item = NULL, *next = NULL;
  uint32_t i = 0;
  if(cache->items) {
    for(; i < cache->hash_table_size; i++) {
      item = cache->items[i];    
      while(item) {
        next = (lru_cache_item *) item->next;
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
      return LRU_CACHE_PTHREAD_ERROR;
    }
  }
  free(cache);
  
  return LRU_CACHE_NO_ERROR;
}


lru_cache_error lru_cache_set(lru_cache *cache, void *key, uint32_t key_length, void *value, uint32_t value_length) {
  test_for_missing_cache();
  test_for_missing_key();
  test_for_missing_value();
  test_for_value_too_large();
  lock_cache();
  
  // if we don't have enough space available for this value, repeatedly remove the oldest item until we do
  while(cache->free_memory < value_length)
    lru_cache_remove_lru_item(cache);
  
  unlock_cache();
  return LRU_CACHE_NO_ERROR;
}


lru_cache_error lru_cache_get(lru_cache *cache, void *key, uint32_t key_length, void **value) {
  test_for_missing_cache();
  test_for_missing_key();
  lock_cache();
  
  // loop until we find the item, or hit the end of a chain
  uint32_t hash_index = lru_cache_hash(cache, key, key_length);
  lru_cache_item *item = cache->items[hash_index];
  
  while(item && lru_cache_cmp_keys(item, key, key_length))
    item = (lru_cache_item *) item->next;
  
  if(item) {
    *value = item->value;
    item->access_count = ++cache->access_count;
  } else {
    *value = NULL;
  }
  
  unlock_cache();
  return LRU_CACHE_NO_ERROR;
}


lru_cache_error lru_cache_delete(lru_cache *cache, void *key, uint32_t key_length) {
  test_for_missing_cache();
  test_for_missing_key();
  lock_cache();
  
  // loop until we find the item, or hit the end of a chain
  lru_cache_item *item = NULL, *prev = NULL;
  uint32_t hash_index = lru_cache_hash(cache, key, key_length);
  item = cache->items[hash_index];
  
  while(item && lru_cache_cmp_keys(item, key, key_length)) {
    prev = item;
    item = (lru_cache_item *) item->next;
  }
  
  if(item) {
    lru_cache_remove_item(cache, prev, item, hash_index);
  }
  
  unlock_cache();
  return LRU_CACHE_NO_ERROR;
}
