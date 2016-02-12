#ifndef sem_H_
#define sem_H_

#define N_PROC 64

int proc_num;

struct sem {
	char lock;
    int count;
    int status[N_PROC];
    int id[N_PROC];
};

void sem_init( struct sem *s, int count );
int sem_try( struct sem *s );
void sem_wait( struct sem *s );
void sem_inc( struct sem *s );

#endif
