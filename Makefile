.PHONY: run

build:
	gcc main.c -o main.o

run: build;
	./main.o < smds/smd "$(p)"

run_test: build;
	valgrind ./main.o < smds/smd "$(p)"
