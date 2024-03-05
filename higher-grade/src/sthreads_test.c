#include <stdlib.h>   // exit(), EXIT_FAILURE, EXIT_SUCCESS
#include <stdio.h>    // printf(), fprintf(), stdout, stderr, perror(), _IOLBF
#include <stdbool.h>  // true, false
#include <limits.h>   // INT_MAX
#include <unistd.h>   // usleep(), sleep()

#include "sthreads.h" // init(), spawn(), yield(), done()

/*******************************************************************************
                   Functions to be used together with spawn()

    You may add your own functions or change these functions to your liking.
********************************************************************************/

/* Prints the sequence 0, 1, 2, .... INT_MAX over and over again.
 */
void numbers() {
  int n = 0;
  while (true) {
    printf(" n = %d\n", n);
    n = (n + 1) % (INT_MAX);
    //if (n > 3) done();
    yield();
  }
}

/* Prints the sequence a, b, c, ..., z over and over again.
 */
void letters() {
  char c = 'a';
  while (true) {
      printf(" c = %c\n", c);
      if (c == 'f') done();
      yield();
      c = (c == 'z') ? 'a' : c + 1;
    }
}

/* Calculates the nth Fibonacci number using recursion.
 */
long fib(long n) {
  switch (n) {
  case 0:
    return 0;
  case 1:
    return 1;
  default:
    return fib(n-1) + fib(n-2);
  }
}

/* Print the Fibonacci number sequence over and over again.

   https://en.wikipedia.org/wiki/Fibonacci_number

   This is deliberately an unnecessary slow and CPU intensive
   implementation where each number in the sequence is calculated recursively
   from scratch.
*/

void fibonacci_slow_a() {
  long n = 30;
  long f;
  while (n<41) {
    f = fib(n);
    if (f < 0) {
      // Restart on overflow.
      n = 0;
    }
    printf(" A: slow_fib(%02ld) = %ld\n", n, fib(n));
    n = (n + 1) % INT_MAX;
  }
  done();
}

void fibonacci_slow_b() {
  long n = 30;
  long f;
  while (n<44) {
    f = fib(n);
    if (f < 0) {
      // Restart on overflow.
      n = 0;
    }
    printf(" B: slow_fib(%02ld) = %ld\n", n, fib(n));
    n = (n + 1) % INT_MAX;
  }
  done();
}

/* Print the Fibonacci number sequence over and over again.

   https://en.wikipedia.org/wiki/Fibonacci_number

   This implementation is much faster than fibonacci().
*/
void fibonacci_fast() {
  int a = 0;
  int b = 1;
  int n = 0;
  int next = a + b;

  while(true) {
    printf(" fast_fib(%02d) = %d\n", n, a);
    next = a + b;
    a = b;
    b = next;
    n++;
    if (a < 0) {
      // Restart on overflow.
      a = 0;
      b = 1;
      n = 0;
    }
  }
}

/* Prints the sequence of magic constants over and over again.

   https://en.wikipedia.org/wiki/Magic_square
*/
void magic_numbers() {
  int n = 3;
  int m;
  while (true) {
    m = (n*(n*n+1)/2);
    if (m > 0) {
      printf(" magic(%d) = %d\n", n, m);
      if (n == 8) done();
      n = (n+1) % INT_MAX;
    } else {
      // Start over when m overflows.
      n = 3;
    }
    yield();
  }
}

/*******************************************************************************
                                     main()

            Here you should add code to test the Simple Threads API.
********************************************************************************/


int main(){
  puts("\n==== Test program for the Simple Threads API ====\n");

  init(); // Initialization
  puts("\n==== Testing join ====\n");
  tid_t thrd = spawn(letters);
  join(thrd);
  puts("is c = f?");

  puts("\n==== Testing yield on two threads ====\n");
  tid_t thrd1 = spawn(magic_numbers);
  tid_t thrd2 =  spawn(letters);

  join(thrd1);
  join(thrd2);

  puts("\n==== Testing timer on two threads ====\n");

  tid_t a = spawn(fibonacci_slow_a);
  tid_t b = spawn(fibonacci_slow_b);

  printf("\nThread A done with tid %d\n", join(a));
  printf("\nThread B done with tid %d\n", join(b));

  puts("\n==== Tests Done! ====\n");
}
