#include <errno.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

#include "sched.h"
#include "adjstack.c"

#define STACK_SIZE  65536 //64k 
#define TRUE        1
#define FALSE       0

struct sched_proc  *currentTask;
struct sched_waitq *runningTask;
unsigned int tickNumbers;
sigset_t block_all;



void sched_init(void (*init_fn)()) {

    // Set up periodic timer and handle SIGVTALRM
    struct timeval timeInterval;
    timeInterval.tv_sec  = 0;
    timeInterval.tv_usec = 100000;

    struct itimerval timer;
    timer.it_interval = timeInterval;
    timer.it_value    = timeInterval;

    tickNumbers = 0;
    setitimer(ITIMER_VIRTUAL, &timer, NULL);
    
    signal(SIGVTALRM, sched_tick);
   
    sigfillset(&block_all);
    //set up runing task
    struct sched_waitq *proc_queue;
    proc_queue = (struct sched_waitq *)malloc(sizeof(struct sched_waitq));
    if (!proc_queue) {
        fprintf(stderr, "Cannot locate memory space for process queue %s\n",strerror(errno));
        exit(1);
    }
    runningTask = proc_queue;

    //create new process
    struct sched_proc *initialTask;
    initialTask = (struct sched_proc *)malloc(sizeof(struct sched_proc));
    if (!initialTask) {
        fprintf(stderr, "Cannot locate memory space for intial process %s\n", strerror(errno));
        exit(1);
    }

    // create new stack space
    void *newsp;
    if ((newsp = mmap(0, STACK_SIZE, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, 0, 0)) == MAP_FAILED) {
        fprintf(stderr, "Mmap function failed: %s\n",strerror(errno));
        exit(1);
    } 
  
    initialTask->allocated = 10; 
    initialTask->state = SCHED_RUNNING;
    initialTask->pid   = getNewPid();
    initialTask->ppid  = initialTask->pid;
    initialTask->stack = newsp;
    initialTask->nice  = 0;

    runningTask->procs[0] = initialTask; 
    runningTask->taskNumbers = 1;
    
    currentTask = initialTask;

    struct savectx current_ctx;
    current_ctx.regs[JB_BP] = newsp + STACK_SIZE;    
    current_ctx.regs[JB_SP] = newsp + STACK_SIZE;    
    current_ctx.regs[JB_PC] = init_fn;  
    restorectx(&current_ctx, 0);
    
    return;
}


int sched_fork() {
    
    sigprocmask(SIG_BLOCK, &block_all, NULL);
    
    int new_pid = getNewPid();
    if (new_pid < 0) {
        fprintf(stderr, "Max numeber of processes have been reached\n");
        return -1;
    }
    
    struct sched_proc *forkProcess; 
    forkProcess = (struct sched_proc *)malloc(sizeof(struct sched_proc));
    if (!forkProcess) {
        fprintf(stderr, "Cannot locate memory space for fork process: %s\n", strerror(errno));
        return -1;
    }
    void *newsp;
    if ((newsp = mmap(0, STACK_SIZE, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, 0, 0)) == MAP_FAILED) {
        fprintf(stderr, "Mmap functiion failed: %s\n", strerror(errno));
        free(forkProcess);
        return -1;
    }  
    memcpy(newsp, currentTask->stack, STACK_SIZE);
 
    forkProcess->state = SCHED_READY;
    forkProcess->pid   = new_pid;
    forkProcess->ppid  = currentTask->pid;      
    forkProcess->stack = newsp;
    forkProcess->nice  = 0;
    forkProcess->cpu_time  = currentTask->cpu_time;
    forkProcess->allocated = currentTask->allocated;
    forkProcess->totalTime = 0;

    adjstack(newsp, newsp + STACK_SIZE, forkProcess->stack - currentTask->stack);
    runningTask->procs[forkProcess->pid - 1] = forkProcess;
    runningTask->taskNumbers++;

    int number = forkProcess->pid; 

    if (!savectx(&forkProcess->context)) { //  Parent
        forkProcess->context.regs[JB_BP] += forkProcess->stack - currentTask->stack;
        forkProcess->context.regs[JB_SP] += forkProcess->stack - currentTask->stack;  
    } else { // Child
        number = 0;
    }

    sigprocmask(SIG_UNBLOCK, &block_all, NULL);
    return number; 
}


void sched_exit(int code) {

    sigprocmask(SIG_BLOCK, &block_all, NULL);
    currentTask->state = SCHED_ZOMBIE;
    currentTask->code  = code;
    runningTask->taskNumbers--;

    int i;
    int ppid = sched_getppid();
    for (i = 0; i < SCHED_NPROC; i++) { 
        if ((runningTask->procs[i])
                && (runningTask->procs[i]->pid == currentTask->ppid) 
                && (runningTask->procs[i]->state == SCHED_SLEEPING)) { // Sleeping Parent
            currentTask = runningTask->procs[i];
            currentTask->state = SCHED_RUNNING;
            restorectx(&(runningTask->procs[i]->context), 1);
        }
    }
   
    sigprocmask(SIG_UNBLOCK, &block_all, NULL);
    sched_switch();
}



int sched_wait(int *exitCode) {
   
    sigprocmask(SIG_BLOCK, &block_all, NULL);
    //child process
    int i;
    int child  = FALSE; 
    for (i = 0; i < SCHED_NPROC; i++) {
        if (runningTask->procs[i]) {
            if ((runningTask->procs[i]->ppid == currentTask->pid)) {
                child = TRUE;
            }   
        }
    }

    if (!child) {
        return -1;  
    }

    int zombie = FALSE; 
    while (1) {    

        
        for (i = 0; i < SCHED_NPROC; i++) {
            if ((runningTask->procs[i]) 
                && (runningTask->procs[i]->state == SCHED_ZOMBIE) 
                && (runningTask->procs[i]->ppid == currentTask->pid)) {
                zombie = TRUE;
                break;
            }
        }

        if (zombie) {
            break;
        }

        currentTask->state = SCHED_SLEEPING;
        sigprocmask(SIG_UNBLOCK, &block_all, NULL);
        sched_switch();
        sigprocmask(SIG_BLOCK, &block_all, NULL);
    }
    
    *exitCode = runningTask->procs[i]->code;
    free(runningTask->procs[i]);

    runningTask->procs[i] = NULL;
    sigprocmask(SIG_UNBLOCK, &block_all, NULL);
    return 0; 
}


void sched_nice(int nice) {
    
    nice = (nice >  19) ?  19 : nice;
    nice = (nice < -20) ? -20 : nice;
    currentTask->nice = nice; 
    return;
}



int sched_getpid() {
    return currentTask->pid;
}

int sched_getppid() {
    return currentTask->ppid;
}

int sched_gettick() {
    return tickNumbers;
}

void updatePriorities() {
    int i;
    for (i = 0; i < SCHED_NPROC; i++) {
        if (runningTask->procs[i]) {
            // Get dynamic priority from the static priority
            int priority = 20 - runningTask->procs[i]->nice;
            priority -= (runningTask->procs[i]->totalTime/(20 - runningTask->procs[i]->nice));
            runningTask->procs[i]->priority = priority;
        }
    }      
}


void sched_switch() {
    if ((currentTask->cpu_time < currentTask->allocated) 
            && !(currentTask->state == SCHED_SLEEPING)
            && !(currentTask->state == SCHED_ZOMBIE)) {
        return;
    }
    sched_ps();
    fprintf(stdout, "Switch called by pid %d\n", currentTask->pid);
    updatePriorities();
    
    if (!((currentTask->state == SCHED_ZOMBIE) || (currentTask->state == SCHED_SLEEPING))) {
        currentTask->state = SCHED_READY;
    }

    int bestProc = 0;
    int bestPriority = INT_MIN; // Unlikely enough for this 

    // Determine new best processs
    int i, sum;
    for (i = 0; i < SCHED_NPROC; i++) {
        if ((runningTask->procs[i]) && (runningTask->procs[i]->state == SCHED_READY)) {
            sum += (20 - runningTask->procs[i]->nice);
            if (runningTask->procs[i]->priority > bestPriority) {
                bestPriority = runningTask->procs[i]->priority;
                bestProc = i;
            }
        }
    } 
 
    struct sched_proc *chosen_proc = runningTask->procs[bestProc];
    fprintf(stdout, "Chose to switch to pid %d\n", chosen_proc->pid);
    
    if (chosen_proc->pid == currentTask->pid) {
        currentTask->cpu_time = 0;
        currentTask->state    = SCHED_READY;
        return;
    }

    if(savectx(&(currentTask->context)) == 0) {  
        currentTask = chosen_proc;
        currentTask->cpu_time = 0;
        currentTask->state    = SCHED_RUNNING;
        restorectx(&(currentTask->context), 1);   
    };
   
    return;
}

void sched_ps() {

    fprintf(stdout, "PID\tPPID\tSTATE\tSTACK\t\tNICE\tPRIORITY\tTIME\tWAIT QUEUE\n");

    // Loop through all tasks and print info
    int i;
    for (i = 0; i < SCHED_NPROC; i++) {
        if (runningTask->procs[i]) {
            struct sched_proc *task = runningTask->procs[i];
            fprintf(stdout, "%d\t%d\t%d\t%p\t", task->pid, task->ppid, task->state, task->stack);
            fprintf(stdout, "%d\t\t%d\t%d", 20-task->nice, task->priority,  task->totalTime);

            if (task->state == SCHED_SLEEPING) {
                fprintf(stdout, "\t%p", runningTask);
            }
            fprintf(stdout, "\n");
        }
    }

    return;
}


void sched_tick() {
    tickNumbers++;
    currentTask->totalTime++;
    currentTask->cpu_time++; 
    sched_switch();
    return;
}

int getNewPid() {
    int i;
    for (i = 0; i < SCHED_NPROC; i++) {
        if (!runningTask->procs[i]) {
            return i+1;
        }
    }

    return -1;
}