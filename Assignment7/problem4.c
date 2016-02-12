#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fifo.h"

#define PROC(x) ((x)>>24)
#define VAL(x) ((x)&((1<<24)-1))

struct fifo *f; 

int main( int argc, char **argv ) {
   
    if( ( f = mmap( NULL, sizeof( struct fifo ), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0 ) ) == MAP_FAILED ) {
        fprintf( stderr, "MMAP failed. %s.\n", strerror( errno ) );
        exit( 1 );
    }
    fifo_init( f );

    if( argc < 3 ) {
        fprintf( stderr, "Usage: problem4 <numprocs> <numrw>\n" );
        exit( 1 );
    } 

    int proc_count = atoi( argv[1] );
    int wr_count = atoi( argv[2] );

    if( proc_count > 64 ) {
        fprintf( stderr, "ERROR: Max procs is 64\n" );
        exit( 1 );
    }
   
    pid_t pid;
    int i;
    for( i = 1; i < proc_count; i++ ) { // Create procs
        if( ( pid = fork() ) == -1 ) {
            fprintf( stderr, "ERROR: Problem forking process. %s.\n", strerror( errno ) );
            exit( 1 );
        }
        if( pid == 0 ) { 
            proc_num = i;
            break; 
        } 
    }


    if( pid != 0 ) { // Parent
        proc_num = 0;
        unsigned long val[proc_count];
        unsigned long rd;
        int rec_proc;
        unsigned long rec_val;

        for( i = 0; i < proc_count; ++i )
            val[i] = -1;

        for( i = 0; i < ( proc_count - 1 ) * wr_count; ++i ) {
            rd = fifo_rd( f );
            rec_proc = ( int ) PROC( rd );
            rec_val = VAL( rd );
            
            if( rec_val != ( val[rec_proc] + 1 ) ) {
                fprintf( stdout, "Test failed in proc_num %d. Recieved val %lu instead of %lu\n", rec_proc, rec_val, val[rec_proc] + 1 ); 
                fprintf( stderr, "ERROR: Test failed." );
                exit( 1 );
            } else
                fprintf( stdout, "RD :: VAL %05lu :: PROC %02d :: RD\n", rec_val, rec_proc );
            
            val[rec_proc] = rec_val;
        }
       
        fprintf( stdout, "Test passed, all sent data recieved.\n");
        return;
    }

    unsigned long wr;
    for( wr = ( proc_num << 24 ); wr < ( wr_count + ( proc_num << 24 ) ); ++wr ) {
        fifo_wr( f, wr );
        fprintf(stdout, "WR :: VAL %05lu :: PROC %02d :: WR\n", VAL( wr ), proc_num );
    }

    return 0;
}
