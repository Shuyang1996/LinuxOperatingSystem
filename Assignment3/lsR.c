#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

void execute( int argc , char *argv[] );
int main (int argc, char *argv[]){
	size_t n = 0;
	char *linePtr = NULL;
	FILE *fp;
	//when there is no specified file, give > to user 
	if( argc ==1){
		fprintf(stdout, ">" );
	}

	if (argc >1 ){
		if((fp=fopen(argv[1], "r"))== NULL){
			fprintf(stderr, "Cannot open file %s for reading. %s.\n", argv[1],strerror(errno) );
			exit (1);
		}
	} else {
		fp = stdin; //(variable provided by <stdio.h> library )
	}

	while(getline(&linePtr,&n,fp)!= -1){
		int newArgc = 0;
		char **newArgv;
		
		//parse arguments from each line below
		char **ArgvList = NULL;
		//#means this line is comment, just ignore this sentence and exit function
		
		if( linePtr[0] == '#' ) {
	    	newArgv = ArgvList;
		}
		// parse out arguments by '\t'
	    char *argument = strtok( linePtr, "\t" );
	    
	    while( argument != NULL ) {
	        ArgvList = realloc( ArgvList, sizeof( char * ) * ++(newArgc) );
	        ArgvList[(newArgc)-1] = argument;
	        argument = strtok( NULL, "\t" );
	    }
	    
	    ArgvList = realloc( ArgvList, sizeof( char * ) * ( (newArgc) + 1 ) );
	    ArgvList[(newArgc)] = 0;
	    newArgv = ArgvList;

	    if(newArgc >0){
			execute(newArgc, newArgv );
		}

		//free the buffer in the next 
		free( newArgv );
	    free( linePtr );
	    linePtr = NULL;
	    n = 0;
	   
	    if( argc == 1 ) {
	        fprintf( stdout, "> " );
	    }
	}

	if( fp != stdin ) {
        fclose( fp );
    }

	return 0;
}

void execute( int argc, char *argv[] ) {
   	//argument counter starts here 
    int counter = 0 ;
    int i;

    // for time report 
    struct timeval t1;
    struct timeval t2;
    struct rusage usage;
    pid_t pid;
    int status;

// count the number of arguments 
    for (i =1; i <argc; i++){
    	if( ( argv[i][1] == '>' &&  argv[i][0] == '2'  && argv[i][2] == '>' ) ||
          ( argv[i][0] == '>' && argv[i][1] =='>' ) ||
          ( argv[i][0] == '<' ) ||
          ( argv[i][0] == '2' && argv[i][1] =='>' ) ||
          ( argv[i][0] == '>' ) ) {
            continue;
        }
        counter++;
    }
    int args = counter;
    //int args = countArgs( argc, argv ); 

    if( args > 0 ) {
        fprintf( stdout, "Executing command %s", argv[0] );
        int i;
        for( i = 1; i < args; i++ ) {
            fprintf( stdout, "\"%s\"", argv[i] );
            if( i != args ) {
                fprintf( stdout, ", " ); 
            } else {
                fprintf( stdout, ".\n" );
            }
        }
    } else {
        fprintf( stdout, "Executing command %s with no arguments.\n", argv[0] );
    }

    switch( pid = fork() ) {
        case -1: {
            fprintf( stderr, "%s fork failed.\n", argv[0] );
            exit( 1 );
            break;
        }
        case 0: {
            int i = argc - 1;
            while( 1 ) {
                int std = -1;
                int fd;
                char *file = NULL;

                if( argv[i][0] == '2' && argv[i][1] == '>' && argv[i][2] == '>' ) {
                    file = strstr( argv[i], "2>>" );
                    file += 3;
                    std  = STDERR_FILENO;
                    fd = open( file, O_WRONLY | O_APPEND | O_CREAT, S_IREAD | S_IWRITE );

               	} else if( argv[i][0] == '>' && argv[i][1] =='>' ) {
                    file = strstr( argv[i], ">>" );
                    file += 2;
                    std  = STDOUT_FILENO;
                    fd = open( file, O_WRONLY | O_APPEND | O_CREAT, S_IREAD | S_IWRITE );

                } else if( argv[i][0] == '2' && argv[i][1] =='>' ) {
                    file = strstr( argv[i], "2>" );
                    file += 2;
                    std  = STDERR_FILENO;
                    fd = open( file, O_WRONLY | O_TRUNC | O_CREAT, S_IREAD | S_IWRITE );

                } else if( argv[i][0] == '<' ) {
                    file = strstr( argv[i], "<" );
                    file += 1;
                    std = STDIN_FILENO;
                    fd = open( file, O_RDONLY );

                } else if( argv[i][0] == '>' ) {
                    file = strstr( argv[i], ">" );
                    file += 1;
                    std  = STDOUT_FILENO;
                    fd = open( file, O_WRONLY | O_TRUNC | O_CREAT, S_IREAD | S_IWRITE );
                } else {
                    break;
                }

                if( fd < 0 || std < 0 ) {
                    fprintf( stderr, "Warning: Cannot open file %s. %s.\n", file, strerror( errno ) );
                    exit( 1 );
                }

                dup2( fd, std );
                close( fd );
                argv[i--] = '\0';
            }

            execvp( argv[0], argv );
            break;
        }

        default:  {
            if( wait3( &status, 0, &usage ) == -1 ) {
                fprintf( stderr, "Warning: Wait3 failed.\n" );
                exit( 1 );
            }

            fprintf( stdout, "Command returned with return code %d,\n", WEXITSTATUS( status ) );
            fprintf( stdout, "consuming %ld.%06d real, %ld.%.06d user, and %ld.%.06d system seconds.\n",
                t2.tv_sec - t1.tv_sec, t2.tv_usec - t1.tv_usec,
                usage.ru_utime.tv_sec, usage.ru_utime.tv_usec,
                usage.ru_stime.tv_sec, usage.ru_stime.tv_usec );
            break;
        }
    }

    return;
}

