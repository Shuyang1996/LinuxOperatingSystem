#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
	char buf[] = "Hello world!\n";
	if( write( 1, buf, strlen( buf ) ) < 0 ) {
		fprintf( stderr, "ERROR: Cannot write to stdin. %s.\n", strerror( errno ) );
		exit(1);
	}; 

	return 0;
}
