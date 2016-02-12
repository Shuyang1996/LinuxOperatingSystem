
#ifndef SCHED_H
#define SCHED_H

#include "savectx.h"

#define STACK_SIZE      65536
#define SCHED_NPROC     511  
#define SCHED_ZOMBIE    -1
#define SCHED_READY     0
#define SCHED_RUNNING   1
#define SCHED_SLEEPING  2

struct sched_proc {
    int pid;                // Processes ID number
    int ppid;               // PID of parent
    int cpu_time;           // cpu time currently running
    int allocated;          // Allocated process time
    int nice;               // Nice Value between -20 and 19
    int priority;           // Static priority between 0 and 39
    int totalTime;          // Total processing time 
    int state;              // Current state of task
    int code;               // Exit code
    void *stack;            // stack address
    struct savectx context; // To restore registers
};


struct sched_waitq {
    struct sched_proc *procs[SCHED_NPROC];
    int taskNumbers;
};

int sched_getpid();

int sched_getppid();

void sched_init(void (*init_fn)());

int sched_fork();

void sched_exit(int code);

int sched_wait(int *exit_code);

void sched_nice(int niceval);

int sched_gettick();

void sched_ps();

void sched_switch();

void sched_tick();

#endif //SCHED_H