example: example.c lruc.c lruc.h
	gcc example.c lruc.c -o example

test: test.c lruc.c lruc.h
	gcc test.c lruc.c -o test
	./test
	rm test
