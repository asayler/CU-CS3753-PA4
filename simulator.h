/*===================================================
   header for CS3753 PA4: memory lookahead
  ===================================================*/ 

#define TRUE  1
#define FALSE 0

#define MAXPROCPAGES 20 	/* max pages per individual process */ 
#define MAXPROCESSES 20 	/* max number of processes in runqueue */ 
#define PAGESIZE 128 		/* size of an individual page */ 
#define PAGEWAIT 100 		/* wait for paging in */ 
#define PHYSICALPAGES 100	/* number of available physical pages */ 
#define MAXPC (MAXPROCPAGES*PAGESIZE) /* largest PC value */ 

typedef struct pentry { 
   long active; 
   long pc; 
   long npages; 
   long pages[MAXPROCPAGES]; /* 0 if not allocated, 1 if allocated */ 
} Pentry ; 

int pageout(int process, int page); 
int pagein (int process, int page); 
void pageit(Pentry q[MAXPROCESSES]); 
