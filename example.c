#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <err.h>
#include "lruc.h"

#define CACHE_SIZE  (8 * 1024)  // 8k
#define AVG_SIZE    (2 * 1024)  // 2k

int main(void) {
  int error = 0;
  char *get = NULL;
  
  // we need to malloc these values because they will be free'd when they're deleted
  char *key = (char *) malloc(5);
  strcpy(key, "test");  
  char *val = (char *) malloc(6);
  strcpy(val, "value");
  
  // create a new cache
  printf("Creating a cache\n");
  lruc *cache = lruc_new(CACHE_SIZE, AVG_SIZE);
  
  printf("Setting a value\n");
  if(error = lruc_set(cache, key, 5, val, 6))
    errx(1, "Error in set: %i\n", error);
  
  // get
  printf("Getting a value\n");
  if(error = lruc_get(cache, "test", 5, (void **)(&get)))
    errx(1, "Error in get: %i\n", error);
  
  if(get)
    printf("Value returned was: %s\n", get);
  else
    printf("No value was returned\n");
  
  // delete
  printf("Deleting a value\n");
  if(error = lruc_delete(cache, key, 5))
    errx(1, "Error in get: %i\n", error);
    
  // get again
  printf("Getting a value\n");
  if(error = lruc_get(cache, "test", 5, (void **)(&get)))
    errx(1, "Error in get: %i\n", error);
  
  if(get)
    printf("Value returned was: %s\n", get);
  else
    printf("No value was returned\n");
  
  printf("Freeing the cache\n");
  if(error = lruc_free(cache))
    errx(1, "Error in free: %i\n", error);
}
