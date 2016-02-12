#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "sem.h"

int main() {
    // Open file
    int fd;
    if( ( fd = open( "file.txt", O_RDWR|O_CREAT, 0644 ) ) < 0 ) {
        fprintf( stderr, "Error opening \"file.txt\": %s\n", strerror( errno ) );
        exit( 1 );
    }

    char str[100] = "Test string to put some text on the first page.";
    if( write( fd, str, 100 ) < 0 ) {
        fprintf( stderr, "Error writing to \"file.txt\": %s\n", strerror( errno ) );
        exit( 1 );
    }

    int *map;
    // call mmap on file
    if( ( map = mmap( NULL, (size_t) 10, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0 ) ) == MAP_FAILED ) {
        fprintf( stderr, "Error calling mmap: %s\n", strerror( errno ) );
        exit( 1 );
    }
    map[0] = 0; // Initialize an int 

    if( close( fd ) < 0 ) {
        fprintf( stderr, "Error calling close: %s\n", strerror( errno ) );
        exit( 1 );
    }

    int num = 0;
    pid_t pid;
    while( num++ < 10 ) {
        if( ( pid = fork() ) == 0 )
            break;
        else if( pid == -1 ) {
            fprintf( stderr, "Error calling fork: %s\n", strerror( errno ) );
            exit( 1 );
        }
    }

    int i = 100000;
    while( i-- )
        map[0]++;

    if( pid != 0 ) {
        wait( NULL );
        printf( "%d, should be %d\n", map[0], 1000000 );

        if( remove( "file.txt" ) < 0 ) {
            fprintf( stderr, "Error removing \"file.txt\": %s\n", strerror( errno ) );
            exit( 1 );
        }
    }

    return 0;
}
