#ifndef FIFO_H_
#define FIFO_H_

#include "sem.h"

#define MYFIFO_BUFSIZ 4096

struct fifo {
    unsigned long data[MYFIFO_BUFSIZ];
    int head;
    int tail;
    struct sem rd;
    struct sem wr;
    struct sem access;
};

void fifo_init( struct fifo *f );
void fifo_wr( struct fifo *f, unsigned long d );
unsigned long fifo_rd( struct fifo *f );

#endif
