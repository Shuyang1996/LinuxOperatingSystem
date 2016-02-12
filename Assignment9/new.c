/*
 * Christian Sherland
 * 12-1-13
 * ECE 357 - Problem Set 9
 *
 * sched.c
 *  Implementation of the scheduler
 *
 */

#include <sys/time.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

#include "sched.h"
#include "adjstack.c"

#define STACK_SIZE  65536
#define TRUE        1
#define FALSE       0

struct sched_proc  *current_proc;
struct sched_waitq *running_procs;
unsigned int num_ticks;
sigset_t block_all;



/*
 * getNewPid()
 *  Helper function to determine the 
 *  lowest available PID
 */
int getNewPid() {
    int i;
    for (i = 0; i < SCHED_NPROC; i++) {
        if (!running_procs->procs[i]) {
            return i+1;
        }
    }

    return -1;
}



void sched_init(void (*init_fn)()) {

    // Set up periodic timer and handle SIGVTALRM
    struct timeval interval;
    interval.tv_sec  = 0;
    interval.tv_usec = 100000;

    struct itimerval timer;
    timer.it_interval = interval;
    timer.it_value    = interval;

    num_ticks = 0;
    setitimer(ITIMER_VIRTUAL, &timer, NULL);
    signal(SIGVTALRM, sched_tick);
   
    // Set up signal masks needed
    sigfillset(&block_all);

    // Set up running task queue
    struct sched_waitq *proc_queue;
    proc_queue = (struct sched_waitq *)malloc(sizeof(struct sched_waitq));
    if (!proc_queue) {
        fprintf(stderr, "Error: mallocing process queue\n");
        exit(1);
    }
    running_procs = proc_queue;

    // Create new process
    struct sched_proc *initial_proc;
    initial_proc = (struct sched_proc *)malloc(sizeof(struct sched_proc));
    if (!initial_proc) {
        fprintf(stderr, "Error: mallocing initial process\n");
        exit(1);
    }

    // Set up new stack space
    void *newsp;
    if ((newsp = mmap(0, STACK_SIZE, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, 0, 0)) == MAP_FAILED) {
        fprintf(stderr, "Error: mmap failed\n");
        exit(1);
    } 
  
    initial_proc->allocated = 10; 
    initial_proc->state = SCHED_RUNNING;
    initial_proc->pid   = getNewPid();
    initial_proc->ppid  = initial_proc->pid;
    initial_proc->stack = newsp;
    initial_proc->nice  = 0;

    running_procs->procs[0] = initial_proc;
    running_procs->n_procs = 1;
    current_proc = initial_proc;
   
    // Start running at init function
    struct savectx current_ctx;
    current_ctx.regs[JB_BP] = newsp + STACK_SIZE;    
    current_ctx.regs[JB_SP] = newsp + STACK_SIZE;    
    current_ctx.regs[JB_PC] = init_fn;  
    restorectx(&current_ctx, 0);
    
    return;
}



int sched_fork() {

    // Critical region
    sigprocmask(SIG_BLOCK, &block_all, NULL);
    
    int new_pid = getNewPid();
    if (new_pid < 0) {
        fprintf(stderr, "Error: max num procs reached\n");
        return -1;
    }

    // Create new process
    struct sched_proc *fork_proc;
    fork_proc = (struct sched_proc *)malloc(sizeof(struct sched_proc));
    if (!fork_proc) {
        fprintf(stderr, "Error: mallocing forked process\n");
        return -1;
    }
    
    // Set up new stack space
    void *newsp;
    if ((newsp = mmap(0, STACK_SIZE, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, 0, 0)) == MAP_FAILED) {
        fprintf(stderr, "Error: mmap failed\n");
        free(fork_proc);
        return -1;
    }  
    memcpy(newsp, current_proc->stack, STACK_SIZE);
 
    fork_proc->state = SCHED_READY;
    fork_proc->pid   = new_pid;
    fork_proc->ppid  = current_proc->pid;      
    fork_proc->stack = newsp;
    fork_proc->nice  = 0;
    fork_proc->cpu_time  = current_proc->cpu_time;
    fork_proc->allocated = current_proc->allocated;
    fork_proc->accumulated = 0;

    // Fix the stack and make it runnable
    adjstack(newsp, newsp + STACK_SIZE, fork_proc->stack - current_proc->stack);
    running_procs->procs[fork_proc->pid - 1] = fork_proc;
    running_procs->n_procs++;

    int ret = fork_proc->pid; 

    if (!savectx(&fork_proc->context)) { //  Parent
        fork_proc->context.regs[JB_BP] += fork_proc->stack - current_proc->stack;
        fork_proc->context.regs[JB_SP] += fork_proc->stack - current_proc->stack;  
    } else { // Child
        ret = 0;
    }

    sigprocmask(SIG_UNBLOCK, &block_all, NULL);
    return ret; 
}


void sched_exit(int code) {

    // Terminate process and store code
    sigprocmask(SIG_BLOCK, &block_all, NULL);
    current_proc->state = SCHED_ZOMBIE;
    current_proc->code  = code;
    running_procs->n_procs--;

    // Check for a parent and wake it up
    int i;
    int ppid = sched_getppid();
    for (i = 0; i < SCHED_NPROC; i++) { 
        if ((running_procs->procs[i])
                && (running_procs->procs[i]->pid == current_proc->ppid) 
                && (running_procs->procs[i]->state == SCHED_SLEEPING)) { // Sleeping Parent
            current_proc = running_procs->procs[i];
            current_proc->state = SCHED_RUNNING;
            restorectx(&(running_procs->procs[i]->context), 1);
        }
    }
   
    sigprocmask(SIG_UNBLOCK, &block_all, NULL);
    sched_switch();
}



int sched_wait(int *exit_code) {
   
    sigprocmask(SIG_BLOCK, &block_all, NULL);
    // Check for children
    int i;
    int child  = FALSE; // Will be PID of child if exists
    for (i = 0; i < SCHED_NPROC; i++) {
        if (running_procs->procs[i]) {
            if ((running_procs->procs[i]->ppid == current_proc->pid)) {
                child = TRUE;
            }   
        }
    }

    if (!child) {
        return -1;  
    }

    // Sleep if not zombie children
    int zombie = FALSE; 
    while (1) {    

        // Check for zombie child
        for (i = 0; i < SCHED_NPROC; i++) {
            if ((running_procs->procs[i]) 
                && (running_procs->procs[i]->state == SCHED_ZOMBIE) 
                && (running_procs->procs[i]->ppid == current_proc->pid)) {
                zombie = TRUE;
                break;
            }
        }

        if (zombie) {
            break;
        }

        current_proc->state = SCHED_SLEEPING;
        sigprocmask(SIG_UNBLOCK, &block_all, NULL);
        sched_switch();
        sigprocmask(SIG_BLOCK, &block_all, NULL);
    }
    
    *exit_code = running_procs->procs[i]->code;
    free(running_procs->procs[i]);
    running_procs->procs[i] = NULL;
    sigprocmask(SIG_UNBLOCK, &block_all, NULL);
    return 0; // Success
}



void sched_nice(int niceval) {
    // Clamp input and set value
    niceval = (niceval >  19) ?  19 : niceval;
    niceval = (niceval < -20) ? -20 : niceval;
    current_proc->nice = niceval; 
    return;
}



int sched_getpid() {
    return current_proc->pid;
}



int sched_getppid() {
    return current_proc->ppid;
}



int sched_gettick() {
    return num_ticks;
}



void updatePriorities() {
    int i;
    for (i = 0; i < SCHED_NPROC; i++) {
        if (running_procs->procs[i]) {
            // Get dynamic priority from the static priority
            int priority = 20 - running_procs->procs[i]->nice;
            priority -= (running_procs->procs[i]->accumulated/(20 - running_procs->procs[i]->nice));
            running_procs->procs[i]->priority = priority;
        }
    }      
}



void sched_ps() {

    fprintf(stdout, "PID\tPPID\tSTATE\tSTACK\t\tNICE\tPRIORITY\tTIME\tWAIT QUEUE\n");

    // Loop through all tasks and print info
    int i;
    for (i = 0; i < SCHED_NPROC; i++) {
        if (running_procs->procs[i]) {
            struct sched_proc *task = running_procs->procs[i];
            fprintf(stdout, "%d\t%d\t%d\t%p\t", task->pid, task->ppid, task->state, task->stack);
            fprintf(stdout, "%d\t\t%d\t%d", 20-task->nice, task->priority,  task->accumulated);

            if (task->state == SCHED_SLEEPING) {
                fprintf(stdout, "\t%p", running_procs);
            }
            fprintf(stdout, "\n");
        }
    }

    return;
}



void sched_switch() {

    // Check if update is needed    
    if ((current_proc->cpu_time < current_proc->allocated) 
            && !(current_proc->state == SCHED_SLEEPING)
            && !(current_proc->state == SCHED_ZOMBIE)) {
        return;
    }
    
    sched_ps();
    fprintf(stdout, "Switch called by pid %d\n", current_proc->pid);
    updatePriorities();
    
    if (!((current_proc->state == SCHED_ZOMBIE) || (current_proc->state == SCHED_SLEEPING))) {
        current_proc->state = SCHED_READY;
    }

    int bestProc = 0;
    int bestPriority = INT_MIN; // Unlikely enough for this 

    // Determine new best process
    int i, sum;
    for (i = 0; i < SCHED_NPROC; i++) {
        if ((running_procs->procs[i]) && (running_procs->procs[i]->state == SCHED_READY)) {
            sum += (20 - running_procs->procs[i]->nice);
            if (running_procs->procs[i]->priority > bestPriority) {
                bestPriority = running_procs->procs[i]->priority;
                bestProc = i;
            }
        }
    } 
 
    struct sched_proc *chosen_proc = running_procs->procs[bestProc];
    fprintf(stdout, "Chose to switch to pid %d\n", chosen_proc->pid);
    
    if (chosen_proc->pid == current_proc->pid) {
        current_proc->cpu_time = 0;
        current_proc->state    = SCHED_READY;
        return;
    }

    // Save current context and put the current
    // process on the wait queue
    if(savectx(&(current_proc->context)) == 0) {  
        // Make the new proc the current proc
        // and restore its context
        current_proc = chosen_proc;
        current_proc->cpu_time = 0;
        current_proc->state    = SCHED_RUNNING;
        restorectx(&(current_proc->context), 1);   
    };
   
    return;
}



void sched_tick() {
    num_ticks++;
    current_proc->accumulated++;
    current_proc->cpu_time++; 
    sched_switch();
    return;
}