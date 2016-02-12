#include <stdio.h>
#include <stdlib.h>


#include "fifo.h"

void fifo_init( struct fifo *f ) {
    f->head = 0;
    f->tail = 0;

    sem_init( &( f->rd ), 0 );
    sem_init( &( f->wr ), MYFIFO_BUFSIZ );
    sem_init( &( f->access ), 1 );

    return;
}

void fifo_wr( struct fifo *f, unsigned long d ) {
    sem_wait( &f->wr );
    sem_wait( &f->access );

    f->data[f->head++] = d;
    f->head %= MYFIFO_BUFSIZ;

    sem_inc( &f->rd );
    sem_inc( &f->access );

    return;
}

unsigned long fifo_rd(struct fifo *f) {
    unsigned long d;

    sem_wait( &f->rd );
    sem_wait( &f->access ); 
 
    d = f->data[f->tail++];
    f->tail %= MYFIFO_BUFSIZ;

    sem_inc( &f->wr );
    sem_inc( &f->access );

    return d;
}
