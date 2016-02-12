#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

#include "sem.h"

int tas( volatile char *lock );

void handle_sigusr1( int sig ) {
    return;
}

void sem_init( struct sem *s, int count ) {
    s->lock  = 0;
    s->count = count;

    int i;
    for( i = 0; i < N_PROC; ++i ) {
        s->status[i] = 0;
        s->id[i] = 0;
    }

    return;
}

int sem_try( struct sem *s ) {
    int res = 0;

    while( tas( &s->lock ) )
        ; 

    if( s->count > 0 ) {
        s->count--;
        res = 1;
    }

    s->lock = 0;

    return res;
}



void sem_wait(struct sem *s) {
    s->id[proc_num] = getpid();

    for(;;) {
        while( tas( &s->lock ) )
            ;

        signal( SIGUSR1, handle_sigusr1 );
        sigset_t oldmask;
        sigset_t newmask;
        sigfillset( &newmask );
        sigdelset( &newmask, SIGUSR1 ); // Don't block SIGUSR1
        sigdelset( &newmask, SIGINT ); // and SIGINT
        sigprocmask( SIG_BLOCK, &newmask, &oldmask);

        if( s->count > 0 ) {
            s->count--;
            s->status[proc_num] = 0;
            s->lock = 0;
            return;
        } else {
            s->lock = 0;
            s->status[proc_num] = 1;
            sigsuspend( &newmask );
        }

        sigprocmask( SIG_UNBLOCK, &newmask, NULL );
    }

    return;
}

void sem_inc(struct sem *s) {
    while( tas( &s->lock ) )
        ; 

    s->count++;
 
    int i;
    for( i = 0; i < N_PROC; ++i ) {
        if( s->status[i] ) {
            kill( s->id[i], SIGUSR1 );
        }
    }
   
    s->lock = 0;
    
    return;
}
