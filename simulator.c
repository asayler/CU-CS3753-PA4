/*
 * File: simulator.c
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
 * 	This is the core simulator code.
 */

#include <stdio.h> 
#include <string.h>
#include <unistd.h>
#include <stdlib.h> 
#include <stdarg.h> 
#include <signal.h>
#include <time.h> 

#include "simulator.h"

FILE *output = NULL; 	/* PC history for statistical analysis */ 
FILE *pages = NULL; 	/* block allocation history */ 
#define MAXPROCESSES 20 /* number of processes in parallel */ 
#define MAXBRANCHES  40	/* number of branches in a program */ 
#define MAXEXITS     10	/* number of maximum exits per program */ 
#define MAXBRINGS   100	/* must be EVEN! data points in branch table */ 

static long sysclock=0; 
static long seed=0; 
static long procs=MAXPROCESSES;

#define LOG_ALWAYS  (1<<0)
#define LOG_LOAD    (1<<1)
#define LOG_BLOCK   (1<<2)
#define LOG_PAGE    (1<<3)
#define LOG_BRANCH  (1<<4)
#define LOG_DEAD    (1<<5)
#define LOG_QUEUE   (1<<9)

#include <stdio.h>
#include <stdarg.h> 
#include <sys/types.h>

// shorthands for assertion handling
#define CHECK(bool)   check((bool),#bool,__FILE__,__LINE__)
#define ASSERT(bool)  assert((bool),#bool,__FILE__,__LINE__)
#define POSIT(bool)   posit((bool),#bool,__FILE__,__LINE__)
#define DIE(reason)   die((reason),__FILE__,__LINE__)
#define CARP(reason) carp((reason),__FILE__,__LINE__)

// always print the result of a test
inline void check(int boolean, char *boolstr, char *file, int line) { 
    if (!boolean) {
        fprintf(stderr,"ERROR: %s failed in line %d of file %s\n",
                boolstr,line,file);
    } else { 
	fprintf(stderr,"%s succeeded in line %d of file %s\n",
                boolstr,line,file);
    }
}

// die if an assertion fails. 
inline void assert(int boolean, char *boolstr, char *file, int line) {
    if (!boolean) {
        fprintf(stderr,"Assertion %s failed in line %d of file %s\n",
                boolstr,line,file);
        exit(1);        /* exit(0) means correct execution! */
    }
}

// report failing assertions without bombing out... keeps running
inline int posit(int boolean, char *boolstr, char *file, int line) {
    if (!boolean) 
        fprintf(stderr,"Assertion %s failed in line %d of file %s\n",
                boolstr,line,file);
    return boolean; 
}

// die on a fatal error
inline void die(char *condition, char *file, int line) {
    fprintf(stderr,"Fatal error: %s at line %d of file %s\n", 
        condition,line,file); 
    exit(1); 
}

// print a non-fatal error 
inline void carp(char *condition, char *file, int line) {
    fprintf(stderr,"Non-fatal error: %s at line %d of file %s\n",
	condition,line,file); 
}

static long log_port=LOG_ALWAYS;  // logging ports for output
static void sim_log(long type, const char *format, ...) { 
    va_list ap; 
    if (log_port&type) { 
	va_start(ap, format);
	fprintf(stderr,"%08ld: ",sysclock); vfprintf(stderr,format,ap); 
	va_end(ap);
    } 
} 

/* keep track of physical page usage */ 
static long pagesavail = PHYSICALPAGES; 

typedef enum { GOTO, FOR, NFOR, IF } BranchType;

/* abstract description of a branch 
   describes qualitative behavior, not actual branching */ 
typedef struct branch { 
   long wherefrom;
   long whereto; 
   BranchType btype; 
   long min, max; 
   double prob; 
   long extent; 
} Branch;

typedef struct program { 
   long size; 
   long nbranches; 
   Branch branches[MAXBRANCHES]; 
   long nexits; 
   long exits[MAXEXITS]; /* which statements are "halt" */ 
} Program; 


// branch context: determines which branch to 
// take next in a probabilistic situation...
typedef struct bcontext { 
   BranchType btype; 
   long boffset;   /* offset into brings array */ 
   long bsize; 	   /* size of brings array */ 
   long bvalue;    /* which branch direction to take */ 
   long bcount;    /* value of current decrement */ 
   long brings[MAXBRINGS]; 
} Bcontext; 

typedef struct process { 
   Program *program; 
   long nbcontexts; 
   Bcontext bcontexts[MAXBRANCHES]; 
   long pc; 	            	/* program counter */ 
   long npages; 
   long pages[MAXPROCPAGES]; 	/* whether page is available */ 
   long blocked[MAXPROCPAGES];	/* whether we've reported page state */ 
   long active;              	/* whether running now */ 
   long compute; 	    	/* number of compute ticks */ 
   long block; 		    	/* number of blocked ticks */ 
   long pid; 			/* unique process number */ 
   long kind; 			/* kind of process from table */ 
} Process;

static Process *processes[MAXPROCESSES]; 

#include "programs.c" 

/* make a binary decision according to a 
   probability distribution */ 
static long binary(double prob) { 
    if (drand48()<prob) return 1; 
    else return 0; 
} 

/* clear a branching engine */ 
static void bcontext_clear( Bcontext *c) { 
    long i; 
    c->bcount=0; 
    c->boffset=0; 
    c->bsize=1; 
    c->bvalue=1; 
    c->brings[0]=1; 
    for (i=1; i<MAXBRINGS; i++) c->brings[i]=0; 
} 

/* initialize a branching engine */ 
static void bcontext_init( Bcontext *c, Branch *b) { 
    long i; 
    c->bcount=0; 
    c->btype=b->btype; 
    if (b->btype==GOTO) { 
	c->boffset=0; 
        c->bsize=1; 
        c->bvalue=1; 
	// c->brings[0]=1; 	 /* old version */ 
        c->brings[0]=0x7fffffff; /* keep going forever */ 
        for (i=1; i<MAXBRINGS; i++) c->brings[i]=0; 
    } else if (b->btype==IF) { 
        long cvalue; 
	c->boffset=0; 
        c->bsize=0; 
        cvalue=c->bvalue=binary(b->prob); 
        c->bcount=0; 
        // compute future values for if statements 
        while (c->bsize<MAXBRINGS)  {
	    if (binary(b->prob)==cvalue) { 
		c->brings[c->bsize]++; 
	    } else { 
		c->bsize++; 
		c->brings[c->bsize]=1; 
                cvalue=!cvalue; 
            } 
	} 
    } else if (b->btype==FOR) { 
	c->boffset=0; 
        c->bvalue=1; 
        c->bsize=0; 
        while (c->bsize<MAXBRINGS) { 
	    if (b->max > b->min) { 
		c->brings[c->bsize++]=lrand48()%(b->max-b->min)+b->min; 
            } else { 
		c->brings[c->bsize++]=b->min; 
            } 
            c->brings[c->bsize++]=1; // one failure to escape loop
        } 
    } else if (b->btype==NFOR) { 
	c->boffset=0; 
        c->bvalue=0; 
        c->bsize=0; 
        while (c->bsize<MAXBRINGS) { 
	    if (b->max > b->min) { 
		c->brings[c->bsize++]=lrand48()%(b->max-b->min)+b->min; 
            } else { 
		c->brings[c->bsize++]=b->min; 
            } 
            c->brings[c->bsize++]=1; // one failure to escape loop
        } 
    } else { 
	fprintf(stderr, "unknown btype %ld\n", (long)(b->btype)); 
    } 
} 

/* decide whether to branch or not */ 
static long bcontext_decide(Bcontext *c) { 
    long ret = c->bvalue; 
    c->bcount++; 
    ASSERT(c->boffset>=0 && c->boffset<MAXBRINGS); 
    if (c->bcount>=c->brings[c->boffset]) { 
	c->boffset = (c->boffset+1)%c->bsize; 
	// ASSERT(c->boffset>0); // no wrapping! 
        c->bcount=0;  
        c->bvalue=!c->bvalue; 
    } 
    return ret; 
} 

static void process_clear(Process *q) { 
   long i; 
   q->pc = 0; 
   q->compute=q->block=0; 
   q->program = NULL; 
   q->pid = -1; 
   q->kind = -1;
   q->nbcontexts = 0; 
   for (i=0; i<MAXBRANCHES; i++) {
       bcontext_clear(q->bcontexts+i); 
   } 
   q->npages = 0; 
   /* no physical pages assigned */ 
   for (i=0; i<MAXPROCPAGES; i++) {
	q->pages[i]=-PAGEWAIT-1; 
	q->blocked[i]=FALSE; // ALC: so simulator will log first access 
   } 
   q->active=FALSE; 
} 

/* load a program into a process */ 
static void process_load(Process *q, Program *p, int pid, int kind) { 
   long i; 
   q->pc = 0; 
   q->compute=q->block=0; 
   q->program = p; 
   q->pid = pid; 
   q->kind = kind; 
   q->nbcontexts = p->nbranches; 
   ASSERT(p->nbranches>=0 && p->nbranches<MAXBRANCHES); 
   for (i=0; i<p->nbranches; i++) {
       bcontext_init(q->bcontexts+i, p->branches+i); 
   } 
   // fprintf(stderr,"actual page size for process is %d\n", (q->program->size+PAGESIZE-1)/PAGESIZE); 
   q->npages = MAXPROCPAGES; 
   for (i=0; i<MAXPROCPAGES; i++) { 
	q->pages[i]=-PAGEWAIT-1; 
 	q->blocked[i]=FALSE; // ALC: so simulator will log first access 
   } 
   /* no physical pages assigned */ 
   q->active=TRUE; 			 /* now running */ 
} 

/* unload a process and release all resources */ 
static void process_unload(int pnum, Process *q) { 
   long i; 
   for (i=0; i<q->npages; i++) 
       if (q->pages[i]>=-PAGEWAIT) { 
	   pagesavail++; q->pages[i]=-PAGEWAIT-1; q->blocked[i]=1;
       } 
   q->active=FALSE; 
   sim_log(LOG_LOAD,"process %2d; pc %04d: unloaded\n",pnum, q->pc); 
} 

/* do a branch if necessary */
static void process_dobranch(int pnum, Process *q, Branch *b, Bcontext *c) {
   if (bcontext_decide(c)) { 
	// must document where we branched from
       if (output) fprintf(output, "%ld,%d,%ld,%ld,%ld,branch_from\n", 
		sysclock, pnum, q->pid, q->kind, q->pc); 
       q->pc = b->whereto; 
	// and where we branched to
       if (output) fprintf(output, "%ld,%d,%ld,%ld,%ld,branch_to\n", 
		sysclock, pnum, q->pid, q->kind, q->pc); 
       sim_log(LOG_BRANCH,"process %2d; pc %04d: branch\n",pnum, q->pc); 
   } else { 
       q->pc++; 
       sim_log(LOG_BRANCH,"process %2d; pc %04d: no branch\n",pnum, q->pc); 
   } 
   if (q->pc<0 || q->pc>=q->program->size) q->pc=0; /* start over */ 
} 

/* compute one step of a process */ 
static long process_step(int pnum, Process *q) { 
   long pc; 
   long page; 
   long max, min; 
   Branch *b; 
   Bcontext *c; 

   if (!q) return FALSE;  
   pc = q->pc; 
   page = q->pc / PAGESIZE; 
   if (!q->active) { return FALSE; } 

   /* if page swapped out, don't allow to run */ 
   if (q->pages[page]!=0) { 
	if (!q->blocked[page]) { 
	    sim_log(LOG_BLOCK,"process=%2d page=%3d blocked\n",pnum,page);
	    if (output) fprintf(output, "%ld,%d,%ld,%ld,%ld,blocked\n", 
		sysclock, pnum, q->pid, q->kind, q->pc); 
	    q->blocked[page]=TRUE; 
	}
	q->block++; return TRUE; 
   } else { 
	if (q->blocked[page]) { 
	    sim_log(LOG_BLOCK,"process=%2d page=%3d unblocked\n",pnum,page);
	    if (output) fprintf(output, "%ld,%d,%ld,%ld,%ld,unblocked\n",
		sysclock,pnum, q->pid, q->kind, q->pc);
	    q->blocked[page]=FALSE; 
        } 
	q->compute++; 
   }

   /* should I exit */ 
   ASSERT(q->program->nexits>=0 && q->program->nexits<=MAXEXITS); 
   min=0; max=q->program->nexits-1; 
   while (min+1<max) { 
       long mid=(min+max)/2; 
       if (pc==q->program->exits[mid]) { 
	    if (output) fprintf(output, "%ld,%d,%ld,%ld,%ld,exit\n", 
		sysclock, pnum, q->pid, q->kind, q->pc);
	    return FALSE; 
       } 
       else if (pc<q->program->exits[mid])  max=mid; 
       else                                 min=mid; 
   } 
   if (pc==q->program->exits[min] || pc==q->program->exits[max]) { 
	if (output) fprintf(output, "%ld,%d,%ld,%ld,%ld,exit\n", 
	    sysclock, pnum, q->pid, q->kind, q->pc);
	return FALSE; 
   } 
   b = q->program->branches; 
   c = q->bcontexts; 
   ASSERT(q->program->nbranches>=0 && q->program->nbranches<=MAXBRANCHES); 
   min=0; max=q->program->nbranches-1; 
   while (min+1<max) { 
       long mid=(min+max)/2; 
       if (pc==b[mid].wherefrom) {
	    process_dobranch(pnum,q,b+mid,c+mid);
	    return TRUE;
       }
       else if (pc<b[mid].wherefrom) max=mid; 
       else                          min=mid; 
   } 
   if (pc==b[min].wherefrom) { process_dobranch(pnum,q,b+min,c+min); return TRUE; } 
   if (pc==b[max].wherefrom) { process_dobranch(pnum,q,b+max,c+max); return TRUE; } 
   q->pc++; /* default action */ 
   if (q->pc<0 || q->pc>q->program->size) { 
	if (output) fprintf(output, "%ld,%d,%ld,%ld,%ld,out_of_range\n", 
	    sysclock, pnum, q->pid, q->kind, q->pc);
	q->pc=0; /* start over */ 
	if (output) fprintf(output, "%ld,%d,%ld,%ld,%ld,restart\n", 
	    sysclock, pnum, q->pid, q->kind, q->pc);
   } 
   return TRUE; 
} 
   

/* public routine: swap one page out */ 
int pageout(int process, int page) { 
    if (process<0 || process>=procs 
     || !processes[process]
     || !processes[process]->active
     || page<0 || page>=processes[process]->npages) 
	return FALSE; 
    if (processes[process]->pages[page]<0) 
	return TRUE; /* on its way out */ 
    if (processes[process]->pages[page]>0) 
	return FALSE; /* not available to swap out */ 
sim_log(LOG_PAGE,"process=%2d page=%3d start pageout\n",process,page);
    if (pages) fprintf(pages,"%ld,%d,%d,%ld,%ld,going\n",
	sysclock,process,page,processes[process]->pid, processes[process]->kind); 
    processes[process]->pages[page]=-1; return TRUE;
} 

/* public routine: swap one page in */ 
int pagein(int process, int page) { 
    if (process<0 || process>=procs 
     || !processes[process]
     || !processes[process]->active
     || page<0 || page>=processes[process]->npages)
	return FALSE; 
    if (processes[process]->pages[page]>=0) 
	return TRUE; /* on its way */ 
    if (pagesavail==0) 
	return FALSE; 
    if (processes[process]->pages[page]>=-PAGEWAIT ) 
	return FALSE; /* not yet out */ 
    sim_log(LOG_PAGE,"process=%2d page=%3d start pagein\n",process,page);
    if (pages) fprintf(pages,"%ld,%d,%d,%ld,%ld,coming\n",
	sysclock,process,page,processes[process]->pid, processes[process]->kind); 
    processes[process]->pages[page]=PAGEWAIT; pagesavail--; return TRUE; 
} 

/*============
   job queue
  ============*/ 

#define QUEUESIZE (PROGRAMS*8)
static long queuetype[QUEUESIZE]; 
static Process queue[QUEUESIZE];
static long queueend; 
static void initqueue() { 
   long i,repeats; 
   for (i=0; i<QUEUESIZE; i++) queuetype[i]=i%PROGRAMS; 
   // for (i=0; i<QUEUESIZE; i++) queuetype[i]=lrand48()%PROGRAMS; 
   for (repeats=0; repeats<10; repeats++) 
       for (i=0; i<QUEUESIZE; i++) { 
	  int j=lrand48()%QUEUESIZE;
	  long temp=queuetype[i]; queuetype[i]=queuetype[j]; queuetype[j]=temp; 
       } 
   for (i=0; i<QUEUESIZE; i++) { 
        process_clear(queue+i); 
	process_load(queue+i,programs+queuetype[i], i, queuetype[i]); 
   } 
   queueend=0; 
} 
static Process * dequeue() { 
   if (queueend<QUEUESIZE) return queue+queueend++; 
   else return NULL; 
} 
static long empty() { return queueend>=QUEUESIZE; } 

/*===========================
   control of all processes 
  ===========================*/ 

static void allprint() { 
    int i,j; 
    fprintf(stderr,"\nprocess  "); 
    for (i=0; i<MAXPROCESSES/2; i++) { 
	if (i) fprintf(stderr," | "); 
	if (processes[i] && processes[i]->active) { 
	    fprintf(stderr,"  %02d",i); 
        } else { 
	    fprintf(stderr,"  --"); 
        }
    } 
    fprintf(stderr,"\n"); 
    fprintf(stderr,"pc       "); 
    for (i=0; i<MAXPROCESSES/2; i++) { 
	if (i) fprintf(stderr," | "); 
	if (processes[i] && processes[i]->active) { 
	    fprintf(stderr,"%04ld",processes[i]->pc); 
        } else { 
	    fprintf(stderr,"----"); 
        }
    } 
    fprintf(stderr,"\n"); 
    for (j=0; j<MAXPROCPAGES; j++) { 
	fprintf(stderr,"page%02d  ",j); 
	for (i=0; i<MAXPROCESSES/2; i++) { 
	    if (i) fprintf(stderr," |"); 
	    if (processes[i] && processes[i]->active) { 
		int pcblock =  processes[i]->pc/PAGESIZE; 
		if (j==pcblock) { 
		    if (processes[i]->pages[j]>0) 
			fprintf(stderr,"*i%3ld",processes[i]->pages[j]); 
		    else if (processes[i]->pages[j]==0) 
			fprintf(stderr,"*=in "); 
		    else if (processes[i]->pages[j]==-100) 
			fprintf(stderr,"*=out"); 
		    else 
			fprintf(stderr,"*o%3ld",100+processes[i]->pages[j]); 
		    // fprintf(stderr,"*%4d",processes[i]->pages[j]); 
	  	} else { 
		    if (processes[i]->pages[j]>0) 
			fprintf(stderr," i%3ld",processes[i]->pages[j]); 
		    else if (processes[i]->pages[j]==0) 
			fprintf(stderr," =in "); 
		    else if (processes[i]->pages[j]==-100) 
			fprintf(stderr," =out"); 
		    else 
			fprintf(stderr," o%3ld",100+processes[i]->pages[j]); 
		    // fprintf(stderr," %4d",processes[i]->pages[j]); 
		} 
	    } else { 
		fprintf(stderr," ----"); 
	    }
	} 
	fprintf(stderr,"\n"); 
    } 
    fprintf(stderr,"----------------------------------------------------------------------------\n"); 
    fprintf(stderr,"process  "); 
    for (i=MAXPROCESSES/2; i<MAXPROCESSES; i++) {
	if (i-MAXPROCESSES/2) fprintf(stderr," | "); 
	if (processes[i] && processes[i]->active) { 
	    fprintf(stderr,"  %02d",i); 
        } else { 
	    fprintf(stderr,"  --"); 
        }
    } 
    fprintf(stderr,"\n"); 
    fprintf(stderr,"pc       "); 
    for (i=MAXPROCESSES/2; i<MAXPROCESSES; i++) {
	if (i-MAXPROCESSES/2) fprintf(stderr," | "); 
	if (processes[i] && processes[i]->active) { 
	    fprintf(stderr,"%04ld",processes[i]->pc); 
        } else { 
	    fprintf(stderr,"----"); 
        }
    } 
    fprintf(stderr,"\n"); 
    for (j=0; j<MAXPROCPAGES; j++) { 
	fprintf(stderr,"page%02d  ",j); 
	for (i=MAXPROCESSES/2; i<MAXPROCESSES; i++) {
	    if (i-MAXPROCESSES/2) fprintf(stderr," |"); 
	    if (processes[i] && processes[i]->active) { 
		int pcblock =  processes[i]->pc/PAGESIZE; 
		if (j==pcblock) { 
		    if (processes[i]->pages[j]>0) 
			fprintf(stderr,"*i%3ld",processes[i]->pages[j]); 
		    else if (processes[i]->pages[j]==0) 
			fprintf(stderr,"*=in "); 
		    else if (processes[i]->pages[j]==-100) 
			fprintf(stderr,"*=out"); 
		    else 
			fprintf(stderr,"*o%3ld",100+processes[i]->pages[j]); 
		    // fprintf(stderr,"*%4d",processes[i]->pages[j]); 
	  	} else {
		    if (processes[i]->pages[j]>0) 
			fprintf(stderr," i%3ld",processes[i]->pages[j]); 
		    else if (processes[i]->pages[j]==0) 
			fprintf(stderr," =in "); 
		    else if (processes[i]->pages[j]==-100) 
			fprintf(stderr," =out"); 
		    else 
			fprintf(stderr," o%3ld",100+processes[i]->pages[j]); 
		    // fprintf(stderr," %4d",processes[i]->pages[j]); 
		} 
	    } else { 
		fprintf(stderr," ----"); 
	    }
	} 
	fprintf(stderr,"\n"); 
    } 
    fprintf(stderr,"----------------------------------------------------------------------------\n"); 
} 

static void endit() { allprint(); exit(0); } 
  
static void allinit () { 
    long i; 
    initqueue(); 
    for (i=0; i<MAXPROCESSES; i++) processes[i]=NULL; 
    for (i=0; i<procs; i++) { 
	// zero out pages from processes
	if (!empty()) {
	    processes[i]=dequeue(); 

	    sim_log(LOG_LOAD,"process %2d; pc %04d: loaded\n",i, processes[i]->pc); 
	    if (output) fprintf(output, "%ld,%ld,%ld,%ld,%ld,load\n", 
		sysclock, i, processes[i]->pid, 
		processes[i]->kind, processes[i]->pc);
	    if (pages) { 
		long j;
		for (j=0; j<MAXPROCPAGES; j++) 
		    fprintf(pages,"%ld,%ld,%ld,%ld,%ld,out\n",
			sysclock,i,j,processes[i]->pid,processes[i]->kind); 
	    } 
	} 
    } 
} 

static void allscore() { 
    int i; 
    int block=0; 
    int compute=0; 
    for (i=0; i<QUEUESIZE; i++) { 
	block+=queue[i].block; 
	compute+=queue[i].compute; 
    } 
    sim_log(LOG_ALWAYS, "simulation ends\n"); 
    sim_log(LOG_ALWAYS, "%d blocked cycles\n",block); 
    sim_log(LOG_ALWAYS, "%d compute cycles\n",compute); 
    sim_log(LOG_ALWAYS, "ratio blocked/compute=%g\n",(double)block/(double)compute); 

} 

static void allstep () { 
    long i; 
    for (i=0; i<procs; i++) { 
	if (!process_step(i,processes[i])) { 
	    if (processes[i] && processes[i]->active) { 
		// document final PC position 
		if (output) fprintf(output, "%ld,%ld,%ld,%ld,%ld,unload\n", 
		    sysclock, i, processes[i]->pid, 
		    processes[i]->kind, processes[i]->pc);
		if (pages) { 
		    long j;
		    for (j=0; j<MAXPROCPAGES; j++) 
			fprintf(pages,"%ld,%ld,%ld,%ld,%ld,out\n",
			    sysclock,i,j,processes[i]->pid, processes[i]->kind); 
		} 
		process_unload(i,processes[i]); 
	    } 
	    processes[i]=NULL; 
            if (!empty()) {
		processes[i]=dequeue();
	        sim_log(LOG_LOAD,"process %2d; pc %04d: loaded\n",i, processes[i]->pc); 
		if (output) fprintf(output, "%ld,%ld,%ld,%ld,%ld,load\n", 
		    sysclock, i, processes[i]->pid, 
		    processes[i]->kind, processes[i]->pc);
	    } 
	} 
    } 
} 

static long alldone () { 
    long i; 
    for (i=0; i<procs; i++) { 
	if (processes[i] && processes[i]->active) return FALSE; 
    } 
    return TRUE; 
} 

static int allblocked() { 
    int allfree=0; 
    int runnable=0; 
    int memwait=0; 
    int freewait=0; 
    int i,stat; 
    for (i=0; i<procs; i++) 
	if (processes[i] && processes[i]->active) { 
	    stat=processes[i]->pages[(int)(processes[i]->pc/PAGESIZE)]; 
	    if (stat>0) memwait++;	/* waiting for swap in */ 
	    else if (stat==0) runnable++; /* ok */ 
	    else if (stat<-PAGEWAIT) allfree++; /* free */
	    else freewait++; /* waiting for swap out */ 
	} 

    if (allfree && !memwait && !runnable && !freewait) { 
	sim_log(LOG_DEAD,"%d process pcs waiting for swap in\n",memwait); 
	sim_log(LOG_DEAD,"%d process pcs runnable\n",runnable); 
	sim_log(LOG_DEAD,"%d process pcs waiting for swap out\n",freewait); 
	sim_log(LOG_DEAD,"%d process pcs swapped out\n",allfree); 
	sim_log(LOG_DEAD, "All needed pages swapped out!\n"); 
	// allprint(); 
	return 1; 
    } else { 
	return 0; 
    } 
} 

static void allage () { 
   long i; 
   for (i=0; i<procs; i++) { 
       if (processes[i] && processes[i]->active) { 
    	   long j; 	
	   for (j=0; j<processes[i]->npages; j++) {
                if (processes[i]->pages[j]==0) ; 
                else if (processes[i]->pages[j]<-PAGEWAIT) ; 
		else if (processes[i]->pages[j]>0) { 
		    processes[i]->pages[j]--; 
		    if (processes[i]->pages[j]==0) { 
			sim_log(LOG_PAGE,"process=%2d page=%3d end   pagein\n",i,j);
			if (pages) fprintf(pages,"%ld,%ld,%ld,%ld,%ld,in\n",
			    sysclock,i,j,processes[i]->pid, processes[i]->kind); 
		    } 
		} else if (processes[i]->pages[j]<0 
                       && processes[i]->pages[j]>=-PAGEWAIT) {
		    processes[i]->pages[j]--; 
		    if(processes[i]->pages[j]<-PAGEWAIT) { 
			sim_log(LOG_PAGE,"process=%2d page=%3d end   pageout\n",i,j);
			if (pages) fprintf(pages,"%ld,%ld,%ld,%ld,%ld,out\n",
			    sysclock,i,j,processes[i]->pid, processes[i]->kind); 
			pagesavail++; 
		    } 
                } 
	    } 
	} 
   } 
} 

static void callyou() { 
    long i,j; 
    Pentry pentry[MAXPROCESSES];
    for (i=0; i<MAXPROCESSES; i++) { 
	if (processes[i]) { 
	    pentry[i].active=processes[i]->active; 
	    pentry[i].pc=processes[i]->pc; 
	    pentry[i].npages = processes[i]->npages; 
	    for (j=0; j<processes[i]->npages; j++) {
		pentry[i].pages[j]=(processes[i]->pages[j]==0); 
	    } 
	    for (; j<MAXPROCPAGES; j++) pentry[i].pages[j]=FALSE; 
        } else { 
	    pentry[i].active=FALSE; 
	    pentry[i].pc=0; 
	    pentry[i].npages = 0; 
	    for (j=0; j<MAXPROCPAGES; j++) pentry[i].pages[j]=FALSE; 
        } 
    } 
    pageit(pentry); 	/* call your routine */ 
} 

int main(int argc, char **argv) { 
    
    long i,errors=0,help=0; 
 
    signal(SIGINT, endit); 
    
    log_port=LOG_ALWAYS; 
    for (i=1; i<argc; i++) { 
	if (strcmp(argv[i],"-help")==0) { 
	    help++;
	} else if (strcmp(argv[i],"-all")==0) { 
	    log_port |= LOG_LOAD|LOG_BLOCK|LOG_PAGE|LOG_BRANCH; 
	} else if (strcmp(argv[i],"-load")==0) { 
	    log_port |= LOG_LOAD; 
	} else if (strcmp(argv[i],"-block")==0) { 
	    log_port |= LOG_BLOCK; 
	} else if (strcmp(argv[i],"-page")==0) { 
	    log_port |= LOG_PAGE; 
	} else if (strcmp(argv[i],"-branch")==0) { 
	    log_port |= LOG_BRANCH; 
	} else if (strcmp(argv[i],"-dead")==0) { 
	    log_port |= LOG_DEAD; 
	} else if (strcmp(argv[i],"-seed")==0) { 
	    if (sscanf(argv[++i],"%ld",&seed)!=1) {
		fprintf(stderr,
			"%s: could not read random seed from command line\n",
			argv[0]); 
		errors++; 
	    } else if (seed<1 || seed>((1<<30)-1)) {
		fprintf(stderr,
			"%s: random seed must be between 1 and %d\n",
			argv[0], (1<<30)-1); 
		errors++; 
	    } 
	} else if (strcmp(argv[i],"-csv")==0) { 
	    output = fopen("output.csv", "w"); 
            if (!output) { 
		fprintf(stderr,
			"%s: could not open output.csv for writing\n",
			argv[0]); 
		errors++; 
	    } 
	    pages = fopen("pages.csv", "w"); 
            if (!pages) { 
		fprintf(stderr,
			"%s: could not open pages.csv for writing\n",
			argv[0]); 
		errors++; 
	    } 
	} else if (strcmp(argv[i],"-procs")==0) { 
	    if (sscanf(argv[++i],"%ld",&procs)!=1) {
		fprintf(stderr,
			"%s: could not read number of processors from command line\n",
			argv[0]); 
		errors++; 
	    } else if (procs<1 || procs>MAXPROCESSES) {
		fprintf(stderr,
			"%s: number of processors must be between 1 and %d\n",
			argv[0], MAXPROCESSES); 
		errors++; 
	    } 
        } else { 
	    fprintf(stderr, "t4: unrecognized argument %s\n", argv[i]); 
	    errors++; 
 	} 
    } 
    if (errors || help) { 
	fprintf(stderr, "%s usage: %s \n", argv[0], argv[0]); 
        fprintf(stderr, "  -all       log everything\n"); 
	fprintf(stderr, "  -load      log loading of processes\n"); 
	fprintf(stderr, "  -unload    log unloading of processes\n"); 
	fprintf(stderr, "  -branch    log program branches\n"); 
	fprintf(stderr, "  -page      log page in and out\n"); 
	fprintf(stderr, "  -seed 512  set random seed to 512\n"); 
	fprintf(stderr, "  -procs 4   run only four processors\n"); 
	fprintf(stderr, "  -dead      detect deadlocks\n"); 
	fprintf(stderr, "  -csv       generate output.csv and pages.csv for graphing\n");
	if(errors) {
	    return EXIT_FAILURE;
	}
	else {
	    return EXIT_SUCCESS;
	}
    } 
    if (seed==0) { 
	seed = (time(NULL)*38491+71831+time(NULL)*time(NULL))&((1<<30)-1); 
    } 
    srand48(seed); 
    sim_log(LOG_ALWAYS,"random seed %d\n", seed); 
    sim_log(LOG_ALWAYS,"using %d processors\n", procs); 
    
    allinit(); 
    while (!alldone()) { // all processes inactive
	allstep(); 	 // advance time one tick; if process done, reload
        allage(); 	 // advance time for page wait variables. 
        callyou(); 	 // call your program
	sysclock++;      // remember new time. 
	allblocked();    // deadlock detection 
    } 
    allscore(); 

    return EXIT_SUCCESS;

} 
