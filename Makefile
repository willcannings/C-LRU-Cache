example: example.c lru_cache.c lru_cache.h
	gcc example.c lru_cache.c -o example

test: test.c lru_cache.c lru_cache.h
	gcc test.c lru_cache.c -o test
	./test
	rm test
