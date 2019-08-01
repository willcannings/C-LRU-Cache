example: example.c lruc.c lruc.h
	gcc queue.c lruc.c example.c -o example

test: test.c lruc.c lruc.h
	gcc test.c lruc.c queue.c -o test
	./test
	rm test
