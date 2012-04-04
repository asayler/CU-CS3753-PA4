/*
 * File: pager.c
 * Author:       Andy Sayler
 *               http://www.andysayler.com
 * Adopted From: Dr. Alva Couch
 *               http://www.cs.tufts.edu/~couch/
 *
 * Project: CSCI 3753 Programming Assignment 4
 * Create Date: Unknown
 * Modify Date: 2012/04/03
 * Description:
 * 	This file contains a simple pageit
 *      implmentation at performs very poorly, but
 *      provided a basic idea of how to user the
 *      simulator interface. Your job is to improve
 *      upon this implmentation.
 */

#include <stdio.h> 

#include "simulator.h"

/* pageit: this is called by the simulator
 * every time something interesting occurs.
 *
 * q: state of every process. 
 */
void pageit(Pentry q[MAXPROCESSES]) { 
    /* Static vars */
    static int tick = 0; // artificial time
    static int timestamps[MAXPROCESSES][MAXPROCPAGES];
    
    /* Local vars */
    int proc;
    int pc;
    int page;
    int oldpage; 

    /* Trivial paging strategy */
    /* Select first active process */ 
    for(proc=0; proc<MAXPROCESSES; proc++) { 
	/* Is process active? */
	if(q[proc].active) {
	    /* Dedicate all work to first active process*/ 
	    pc = q[proc].pc; 		        // program counter for process
	    page = pc/PAGESIZE; 		// page the program counter needs
	    timestamps[proc][page] = tick;	// last access
	    /* Is page swaped-out? */
	    if(!q[proc].pages[page]) {
		/* Try to swap in */
		if(!pagein(proc,page)) {
		    /* If swapping fails, swap out another page */
		    for(oldpage=0; oldpage < q[proc].npages; oldpage++) {
			/* Make sure page isn't one I want */
			if(oldpage != page) {
			    /* Try to swap-out */
			    if(pageout(proc,oldpage)) {
				/* Break loop once swap-out starts*/
				break;
			    } 
			}
		    }
		}
	    }
	    /* Break loop after finding first active process */
	    break;
	}
    } 

    /* advance time for next pageit iteration */
    tick++;
    
    /* Get rid of unused var error for timestamps */
    /* note: remove this once timestamps is used */
    (void) timestamps;

} 

// proc: process to work upon (0-19) 
// page: page to put in (0-19)
// returns
//   1 if pagein started or already started
//   0 if it can't start (e.g., swapping out) 
// int pagein(int proc, int page); 

// proc: process to work upon (0-19)
// page: page to swap out. 
// returns: 
//   1 if pageout started (not finished!) 
//   0 if can't start (e.g., swapping in)
// int pageout(int proc, int page); 

// first strategy: least-recently-used
// make a time variable, count ticks. 
// make an array: what time each page was used for 
// each process: 
// int usedtime[PROCESSES][PAGES]; 
// At any time you need a new page, want to swap 
// out the page that has least usedtime. 

// second strategy: predictive
// track the PC for each process over time
// e.g., in a ring buffer. 
// int pc[PROCESSES][TIMES]; 
// at any time pc[i][0]-pc[i][TIMES-1] are the last TIMES
// locations of the pc. If these are near a page border (up 
// or down, and if there is an idle page, swap it in. 

// Note: all data keeping must be in GLOBAL variables. 
