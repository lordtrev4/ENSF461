.PHONY: all
all: mathserver.out

mathserver.out: mathserver.c
	gcc -g -Wall -o $@ $^ -pthread -lm

clean:
	rm -f mathserver.out
