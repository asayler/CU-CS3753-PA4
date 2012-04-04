# CS3753 - PA4

CC = gcc
CFLAGS = -c -g -Wall -Wextra
LFLAGS = -g -Wall -Wextra

.PHONY: all clean

all: test

test: simulator.o pager.o
	$(CC) $(LFLAGS) $^ -o $@ -lm  

simulator.o: simulator.c programs.c simulator.h
	$(CC) $(CFLAGS) $<

pager.o: pager.c simulator.h 
	$(CC) $(CFLAGS) $<

clean:
	rm -f test
	rm -f *.o
	rm -f *~
	rm -f *.csv
	rm -f *.pdf
	rm -f handout/*~
	rm -f handout/*.log
	rm -f handout/*.aux
