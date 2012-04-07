# CS3753 - PA4

CC = gcc
CFLAGS = -c -g -Wall -Wextra
LFLAGS = -g -Wall -Wextra

.PHONY: all clean

all: test-basic test-lru test-predict

test-basic: simulator.o pager-basic.o
	$(CC) $(LFLAGS) $^ -o $@ -lm  

test-lru: simulator.o pager-lru.o
	$(CC) $(LFLAGS) $^ -o $@ -lm 

test-predict: simulator.o pager-predict.o
	$(CC) $(LFLAGS) $^ -o $@ -lm 

simulator.o: simulator.c programs.c simulator.h
	$(CC) $(CFLAGS) $<

pager-basic.o: pager-basic.c simulator.h 
	$(CC) $(CFLAGS) $<

pager-lru.o: pager-lru.c simulator.h 
	$(CC) $(CFLAGS) $<

pager-predict.o: pager-predict.c simulator.h 
	$(CC) $(CFLAGS) $<

clean:
	rm -f test-basic test-lru test-predict
	rm -f *.o
	rm -f *~
	rm -f *.csv
	rm -f *.pdf
	rm -f handout/*~
	rm -f handout/*.log
	rm -f handout/*.aux
