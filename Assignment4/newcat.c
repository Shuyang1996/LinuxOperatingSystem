#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <setjmp.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>


void pipeBreak_handler(int a){
    siglongjmp(jmp_buf, 1);
}

void interrupt_handler(int bytes , int files){
    fprintf(stderr, "Processed %u bytes in %u files\n", bytes, files - 1);
    exit(0);
}

int main (int argc, char *argv[]){
	int grepPipe[2];
	int morePipe[2];
	int i;
	int pid1;
	int pid2;
	int fp;
	struct sigaction interrupt, breakpipe;

	for(i=2; i<argc; ++i){
		if(pipe(grepPipe)||pipe(morePipe)<0){
			fprintf(stderr, "%s\n", strerror(errno));
			return -1;
		}
		//open file first
		if((fp = open(argv[i],O_RDONLY))== -1){
			fprintf(stderr, "Cannot open file %s %s\n",argv[i],strerror(errno) );
			return -1;
		}
		
		//create the child process for grep
		switch(pid1=fork()){
			case 0:	
				//in child process.
				//redirect stdin to grepPipe[0]
				if(grepPipe[0]!=STDIN_FILENO){
					if(dup2(grepPipe[0],STDIN_FILENO)!=STDIN_FILENO){
						fprintf(stderr, "GrepPipe dup2 failed %s\n",strerror(errno) );
						exit(0);
					}
				}
				if(morePipe[1]!=STDOUT_FILENO){
					if(dup2(morePipe[1],STDOUT_FILENO)!=STDOUT_FILENO){
						fprintf(stderr, "MorePipe dup2 failed %s\n",strerror(errno) );
						exit(0);
					}
				}
				
				if (close(morePipe[1]) == -1){
			        fprintf(stderr, "Cannot close file descriptor 1 %s\n", strerror(errno));
			        exit(-1);
			    }

			    if (close(morePipe[0]) == -1){
			        fprintf(stderr, "Cannot close file descriptor 2 %s\n", strerror(errno));
			        exit(-1);
			    }

			    if (close(grepPipe[0]) == -1){
			        fprintf(stderr, "Cannot close file descriptor 3 %s\n", strerror(errno));
			        exit(-1);
			    }

			    if (close(grepPipe[1]) == -1){
			        fprintf(stderr, "Cannot close file descriptor 4 %s\n", strerror(errno));
			        exit(-1);
			    }
			    if(execlp("grep","grep",argv[1],NULL)== -1){
			    	fprintf(stderr, "grep execution failed %s\n", strerror(errno) );
			    	return -1;
			    }
			    return 0;
				break;
			case -1:
				fprintf(stderr, "Grep fork failed %s\n", strerror(errno));
				return -1;
			
			default:
				break;
		}

		switch(pid2=fork()){
			//create more process
			case 0:
				if(morePipe[0]!=STDIN_FILENO){
					if(dup2(morePipe[0],STDIN_FILENO)!=STDIN_FILENO){
						fprintf(stderr, "MorePipe dup2 failed %s\n",strerror(errno) );
						exit(0);
					}
				}
				if (close(morePipe[1]) == -1){
			        fprintf(stderr, "Cannot close file descriptor 5 %s\n", strerror(errno));
			        exit(-1);
			    }

			    if (close(morePipe[0]) == -1){
			        fprintf(stderr, "Cannot close file descriptor 6 %s\n", strerror(errno));
			        exit(-1);
			    }

			    if (close(grepPipe[0]) == -1){
			        fprintf(stderr, "Cannot close file descriptor 7 %s\n", strerror(errno));
			        exit(-1);
			    }

			    if (close(grepPipe[1]) == -1){
			        fprintf(stderr, "Cannot close file descriptor 8 %s\n", strerror(errno));
			    }

				if (execlp("more", "more", NULL) == -1){
	                fprintf(stderr, "More execution failed%s\n", strerror(errno));
	                return -1;
	            }
            return 0;

			case -1:
				fprintf(stderr, "More fork failed%s\n", strerror(errno));
				return -1;

			default:
				break;
		}

			
		if (close(morePipe[1]) == -1){
	        fprintf(stderr, "Cannot close file descriptor 9 %s\n", strerror(errno));
	        exit(-1);
	    }

	    if (close(morePipe[0]) == -1){
	        fprintf(stderr, "Cannot close file descriptor 10 %s\n", strerror(errno));
	        exit(-1);
	    }

	    if (close(grepPipe[0]) == -1){
	        fprintf(stderr, "Cannot close file descriptor 11 %s\n", strerror(errno));
	        exit(-1);
	    }

		
        //set up signal handler
		interrupt.sa_handler = interrupt_handler(bytes,i);
		breakpipe.sa_handler = pipeBreak_handler;
		if (sigaction(SIGINT, &interrupt, NULL) == -1)
        {
            fprintf(stderr, "Interrupt handler failed: %s\n",strerror(errno) );
            return -1;
        }
        if (sigaction(SIGPIPE, &breakpipe, NULL) == -1)
        {
            fprintf(stderr, "Pipe broken handler failed %s\n", strerror(errno));
           
            return -1;
        }

        //signal redirection
        int judge;
		if (sigsetjmp(jmp_buf, 1) == 0) {
			//write infile to grepPipe[1]
			int MAXLINE = 2048;
			char buffer[MAXLINE];
			int bytesread;
			char* pointer;
			int n;
			int byteswritten;
			int bytes;
			
			do
		    {
		        bytesread = read(fp, buffer, MAXLINE);
		        // handle a partial write
		        pointer = buffer;
		        n = bytesread;
		        while (n > 0)
		        {
		            byteswritten = write(grepPipe[1], pointer, n);
		            if (byteswritten <= 0)
		                judge = -2;
		            pointer += byteswritten;
		            bytes += byteswritten;
		            n -= byteswritten;
		        }
		    }
		    while (bytesread > 0);
		    judge =  bytesread;
		}

		if (judge == -1){
            fprintf(stderr, "Reading failed %s: %s\n",argv[i], strerror(errno));     
          	return -1;
        }
        else if (judge == -2){
            fprintf(stderr, "Writing failed %s: %s\n", argv[i], strerror(errno));          
            return -1;
        }

		if(close(fp) == -1){
			fprintf(stderr, "Cannot close file descriptor 12 %s\n", strerror(errno));
	        exit(-1);
		}	
		if(close(grepPipe[1])) == -1){
			fprintf(stderr, "Cannot close file descriptor 13 %s\n", strerror(errno));
	        exit(-1);
		}
		waitpid(pid2,NULL,0);

	}//closing tag for for loop
	return 0;
}
