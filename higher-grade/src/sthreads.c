/* On Mac OS (aka OS X) the ucontext.h functions are deprecated and requires the
   following define.
*/
#define _XOPEN_SOURCE 700

/* On Mac OS when compiling with gcc (clang) the -Wno-deprecated-declarations
   flag must also be used to suppress compiler warnings.
*/

#include <signal.h> /* SIGSTKSZ (default stack size), MINDIGSTKSZ (minimal
                         stack size) */
#include <stdio.h>  /* puts(), printf(), fprintf(), perror(), setvbuf(), _IOLBF,
                         stdout, stderr */
#include <stdlib.h> /* exit(), EXIT_SUCCESS, EXIT_FAILURE, malloc(), free() */
#include <ucontext.h> /* ucontext_t, getcontext(), makecontext(),
                         setcontext(), swapcontext() */
#include <stdbool.h>  /* true, false */

#include "sthreads.h"

#include <string.h>   // memset()
#include <sys/time.h> // ITIMER_REAL, ITIMER_VIRTUAL, ITIMER_PROF, struct itimerval, setitimer()

/* Stack size for each context. */
#define STACK_SIZE SIGSTKSZ * 100

/*******************************************************************************
                             Global data structures

                Add data structures to manage the threads here.
********************************************************************************/
thread_t *threads;
tid_t next_tid;

thread_t *running_thread;
thread_t *last_thread;

/*******************************************************************************
                             Auxiliary functions

                      Add internal helper functions here.
********************************************************************************/

void timer_signal(int signum) {
  printf("Thread %d preempted \n", running_thread->tid);
  yield();
}

void set_timer(int ms){
   struct itimerval timer;
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = ms*1000;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = ms*1000;
  if (setitimer(ITIMER_REAL, &timer, NULL) < 0) {
    perror("Setting timer");
    exit(EXIT_FAILURE);
  };
}



/*******************************************************************************
                    Implementation of the Simple Threads API
********************************************************************************/




int init() { 
   char *new_stack = calloc(STACK_SIZE, sizeof(char));
   next_tid = 0;
   threads = calloc(1, sizeof(thread_t));
   threads->tid = next_tid;
   next_tid++;
   threads->state = running;
   if (getcontext(&threads->ctx) == -1)
   {
      perror("Could not create context for first thread");
      exit(EXIT_FAILURE);
   }
   threads->ctx.uc_stack.ss_sp = new_stack;
   threads->ctx.uc_stack.ss_size = STACK_SIZE;
   threads->next = NULL;
   running_thread = threads;
   last_thread = threads;
   struct sigaction sa;

   memset (&sa, 0, sizeof (sa));
   sa.sa_handler =  timer_signal;
   sigaction (SIGALRM, &sa, NULL);
   set_timer(2);
   return 1; 
   }

tid_t spawn(void (*start)()) {
   char *new_stack = calloc(STACK_SIZE, sizeof(char));

   thread_t *new_thread =  calloc(1, sizeof(thread_t));
   new_thread->tid = next_tid;
   next_tid++;
   new_thread->state = running;
   if (getcontext(&new_thread->ctx) == -1)
   {
      perror("Could not create context thread");
      return -1;
   }
   new_thread->ctx.uc_stack.ss_sp = new_stack;
   new_thread->ctx.uc_stack.ss_size = STACK_SIZE;
   last_thread->next = new_thread;
   last_thread = new_thread;

   makecontext(&(new_thread->ctx), start, 0);

   thread_t *calling_thread = running_thread;
   running_thread->state = ready;
   running_thread = new_thread;
   
   if (swapcontext(&calling_thread->ctx, &new_thread->ctx)) exit(EXIT_FAILURE);
   return new_thread->tid; 
}

void yield() {
   thread_t *calling_thread = running_thread;
   calling_thread->state = ready;
   thread_t *next_thread = calling_thread->next;
   if (next_thread == NULL) next_thread = threads;
   while (next_thread->state != ready)
   {
      next_thread = next_thread->next;
      if (next_thread == NULL) next_thread = threads;
   }
   
   running_thread = next_thread;
   running_thread->state = running;
   
   if (swapcontext(&calling_thread->ctx, &next_thread->ctx)) exit(EXIT_FAILURE);
}

void done() {
   thread_t *waiting_thread = running_thread->next;
   if (waiting_thread == NULL) waiting_thread = threads;
   for (int i = 0; i < next_tid; i++)
   {
      waiting_thread = waiting_thread->next;
      if (waiting_thread == NULL) waiting_thread = threads;

      if (waiting_thread->state == waiting && waiting_thread->waiting_for == running_thread->tid)
      {
         waiting_thread->state = ready;
         waiting_thread->waiting_for = -1;
      }
   }

   thread_t *next_thread = running_thread->next;
   if (next_thread == NULL) next_thread = threads;
   int threads_checked = 1;
   while (next_thread->state != ready)
   {
      next_thread = next_thread->next;
      if (next_thread == NULL) next_thread = threads;
      if (threads_checked == next_tid) exit(EXIT_SUCCESS);
   }
   
   running_thread->state = terminated;
   running_thread = next_thread;
   running_thread->state = running;
   
   if (setcontext(&next_thread->ctx)) 
   {
      exit(EXIT_FAILURE);
   }
}

tid_t join(tid_t thread) {   
   thread_t *terminated_thread = running_thread->next;
   if (terminated_thread == NULL) terminated_thread = threads;
   for (int i = 0; i < next_tid; i++)
   {
      terminated_thread = terminated_thread->next;
      if (terminated_thread == NULL) terminated_thread = threads;

      if (terminated_thread->state == terminated && terminated_thread->tid == thread)
      {
         return thread;
      }
   }
   thread_t *next_thread = running_thread->next;
   if (next_thread == NULL) next_thread = threads;
   while (next_thread->state != ready)
   {
      next_thread = next_thread->next;
      if (next_thread == NULL) next_thread = threads;
   }
   
   thread_t *calling_thread = running_thread;
   calling_thread->state = waiting;
   calling_thread->waiting_for = thread;
   running_thread = next_thread;
   running_thread->state = running;
   
   if (swapcontext(&calling_thread->ctx, &next_thread->ctx)){
      exit(EXIT_FAILURE);
   } 
   return thread;
}