/*
 * File: api-test.c
 * Author:       Andy Sayler
 *               http://www.andysayler.com
 * Adopted From: Dr. Alva Couch
 *               http://www.cs.tufts.edu/~couch/
 *
 * Project: CSCI 3753 Programming Assignment 4
 * Create Date: 2012/04/11
 * Modify Date: 2012/04/11
 * Description:
 * 	This file contains a number of calls to the simulator API
 *      to confirm the behavior of the simulator is specific situations.
 */

#include <stdio.h>
#include <stdlib.h>

#include "simulator.h"

#define MAXITERATIONS 5

void pageit(Pentry q[MAXPROCESSES]) { 
    
    /* Static Vars */
    static int tick = 0;
    static int outTestRun = 0;
    static int inTestRun = 0;
    static int iterations = 0;

    /* Local vars */
    int testProc = 0;
    int testPage = 0;
    int pageinret = -1;
    int pageoutret = -1;
    
    /* All pages are swapped out on start */
    if(q[testProc].pages[testPage]){
	/* Page is swapped in */
	if(!inTestRun){
	    fprintf(stdout, "%4d - %d:%d is swapped in\n", tick, testProc, testPage);
	    fprintf(stdout, "%4d - q[%d].pages[%d] = %ld\n",
		    tick, testProc, testPage, q[testProc].pages[testPage]);
	    
	    pageinret = pagein(testProc, testPage);
	    fprintf(stdout, "%4d - pagein(%d, %d) returns %d\n",
		    tick, testProc, testPage, pageinret);	    

	    fprintf(stdout, "%4d - q[%d].pages[%d] = %ld\n",
		    tick, testProc, testPage, q[testProc].pages[testPage]);

	    pageoutret = pageout(testProc, testPage);
	    fprintf(stdout, "%4d - pageout(%d, %d) returns %d\n",
		    tick, testProc, testPage, pageoutret);
	    if(pageoutret){
		/* Wait for pageout to complete */
		inTestRun = 1;
		outTestRun = 0;
		iterations++;
	    }

	    fprintf(stdout, "%4d - q[%d].pages[%d] = %ld\n",
		    tick, testProc, testPage, q[testProc].pages[testPage]);
	}
    }
    else{
	/* Page is swapped out */
	if(!outTestRun){
	    fprintf(stdout, "%4d - %d:%d is swapped out\n", tick, testProc, testPage);
	    fprintf(stdout, "%4d - q[%d].pages[%d] = %ld\n",
		    tick, testProc, testPage, q[testProc].pages[testPage]);
	    
	    pageoutret = pageout(testProc, testPage);
	    fprintf(stdout, "%4d - pageout(%d, %d) returns %d\n",
		    tick, testProc, testPage, pageoutret);
	    
	    fprintf(stdout, "%4d - q[%d].pages[%d] = %ld\n",
		    tick, testProc, testPage, q[testProc].pages[testPage]);
	    
	    pageinret = pagein(testProc, testPage);
	    fprintf(stdout, "%4d - pagein(%d, %d) returns %d\n",
		    tick, testProc, testPage, pageinret);
	    if(pageinret){
		/* Wait for pagein to complete */
		outTestRun = 1;
		inTestRun = 0;
		iterations++;
	    }else{
		fprintf(stdout, "%4d - pageout in progress...\n", tick);
	    }

	    fprintf(stdout, "%4d - q[%d].pages[%d] = %ld\n",
		    tick, testProc, testPage, q[testProc].pages[testPage]);
	}
    }
   
    /* Run test for I state change iterations */
    if(iterations > MAXITERATIONS){
	fprintf(stdout, "API Test Exiting\n");
	exit(EXIT_SUCCESS);
    }

    tick++;

} 
