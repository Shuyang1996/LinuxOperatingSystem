#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include "sched.h"

void parent() { 
    fprintf(stdout, "Process %d with parent %d start\n", sched_getpid(), sched_getppid());
    
    int i, code;
    for (i = -2; i < 2; i += 1) {
        sched_wait(&code);
        fprintf(stdout, "Process %d exited\n", code);
    }
    return;
}



void child() {
    fprintf(stdout, "Process %d with parent %d start\n", sched_getpid(), sched_getppid());

    long long i;
    for (i = 0; i < 1000000000; i++) {
        if ((i % 100000000) == 0) {
            fprintf(stdout, "Process %d at %d\n", sched_getpid(), i);
        }
    }

    sched_exit(sched_getpid());
}



void test_fn() {
  
    fprintf(stdout, "Successfully made it to the testing function\n");

    int i;
    for (i = -2; i < 2; i += 1) {
        switch(sched_fork()) {
            case -1:
                fprintf(stderr, "Error: fork failed.\n");
                exit(1);

            case 0:
                sched_nice(i);
                child();
                exit(1);
        }
    }

    parent();
    exit(0);
}



void abrt_handler() {
    sched_ps();
    return;
}



int main(int argc, char **argv) {
    struct sigaction sa;
    sa.sa_flags=0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler=abrt_handler;
    sigaction(SIGABRT,&sa,NULL);
    sched_init(test_fn);
    fprintf(stdout, "Should not ever get here.\n");
    return 0;
}