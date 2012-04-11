/*
 * File: simulator.h
 * 
 * Original Author: Dr. Alva Couch
 *                  http://www.cs.tufts.edu/~couch/
 * Modified By:     Andy Sayler
 *                  http://www.andysayler.com
 *
 * Project: CSCI 3753 Programming Assignment 4
 * Create Date: Unknown
 * Modify Date: 2012/04/03
 * Description:
 * 	This is the core simulator header file.
 */

#define TRUE  1
#define FALSE 0

#define MAXPROCPAGES 20 	/* max pages per individual process */ 
#define MAXPROCESSES 20 	/* max number of processes in runqueue */ 
#define PAGESIZE 128 		/* size of an individual page */ 
#define PAGEWAIT 100 		/* wait for paging in */ 
#define PHYSICALPAGES 100	/* number of available physical pages */ 
#define MAXPC (MAXPROCPAGES*PAGESIZE) /* largest PC value */ 

struct pentry {
    long active; 
    long pc; 
    long npages; 
    long pages[MAXPROCPAGES]; /* 0 if not allocated, 1 if allocated */ 
};

typedef struct pentry Pentry; 

/* int pagein (int process, int page)
 *   This pages in the requested page
 * Arguments:
 *   proc: process to work upon (0-19) 
 *   page: page to put in (0-19)
 * Returns:
 *   1 if pagein started, already running, or paged in
 *   0 if it can't start (e.g., swapping out) 
 */
extern int pagein (int process, int page); 

/* int pageout(int process, int page)
 *   This pages out the requested page.
 * Arguments:
 *   proc: process to work upon (0-19)
 *   page: page to swap out. 
 * Returns: 
 *   1 if pageout started, already running, or paged out
 *   0 if can't start (e.g., swapping in)
 */
extern int pageout(int process, int page); 

/* void pageit(Pentry q[MAXPROCESSES])
 *   This is called by the simulator
 *   every time something interesting occurs.
 *   It is where you implement the paging strategy.
 * Arguments:   
 *   q: state of every process
 * Returns:
 *   void 
 */
extern void pageit(Pentry q[MAXPROCESSES]); 
