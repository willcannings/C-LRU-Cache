#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <err.h>
#include "lruc.h"

#define CACHE_SIZE  (8 * 1024)  // 8k
#define AVG_SIZE    (2 * 1024)  // 2k

char *keys[5] = {
  "zero",
  "one",
  "two",
  "three",
  "four"
};

char *values[5] = {
  "zero",
  "one",
  "two",
  "three",
  "four"  
};

void copy_key_val(lruc *cache, char *key, char *val) {
  int error = 0;
  char *new_key = (char *) malloc(strlen(key) + 1);
  strcpy(new_key, key);
  char *new_val = (char *) malloc(strlen(val) + 1);
  strcpy(new_val, val);
  if(error = lruc_set(cache, new_key, strlen(key) + 1, new_val, AVG_SIZE))
    errx(1, "Error in set: %i\n", error);
}

int main(void) {
  int error = 0, i = 0;
  char *get = NULL;
  
  // create a new cache
  printf("Creating a cache\n");
  lruc *cache = lruc_new(CACHE_SIZE, AVG_SIZE);
  
  // stress test inserting the five values 1000 times
  printf("Setting values\n");
  for(; i < 1000000; i++)
    copy_key_val(cache, keys[i % 5], values[i % 5]);
  
  // we should be able to get 4, 3, 2, 1, but not 0
  printf("Getting values\n");
  for(i = 4; i >= 0; i--) {
    printf("Getting: %s\n", keys[i]);
    if(error = lruc_get(cache, keys[i], strlen(keys[i]) + 1, (void **)(&get)))
      errx(1, "Error in get: %i\n", error);

    if(get)
      printf("Value returned was: %s\n", get);
    else
      printf("No value was returned\n");
  }
  
  printf("Freeing the cache\n");
  if(error = lruc_free(cache))
    errx(1, "Error in free: %i\n", error);
}
