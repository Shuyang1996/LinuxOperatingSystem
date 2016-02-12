#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

int main (int argc, char *argv[]){
	int i ;
	long BUFFSIZE = 1024;
	int fdOutput;
	int inputFile;
	int bytesread;
	int buffer[BUFFSIZE];
	int judge = 0;
	int errorMessage;

	if(argc>1){
		for(i =1; i<argc; i++){
			if( strcmp( "-b", argv[i] ) == 0 ){
				BUFFSIZE = atoi(argv[++i]);
			} else if( strcmp( "-o", argv[i] ) == 0 ){
				fdOutput = open(argv[++i], O_TRUNC | O_WRONLY | O_CREAT, 0755);
				if(fdOutput == -1 ){
					fprintf("ERROR opening file %s %s\n", argv[i++],strerror (errno) );
					return -1;
				} 
			} else 
				break;
		}
	} else {
		printf("You have to enter valid inputs\n");
		return (-1);
	}
	
	while(i < argc){
		if(strcmp( argv[i], "-" ) == 0){
			inputFile = 0;
		} else { 
			inputFile = openat(AT_FDCWD,argv[i],O_RDONLY);
			if(inputFile == -1){
				printf( "ERROR %s %s", argv[i], strerror( errno )  );
				return -1;
			}
		} 
		while ((bytesread = read (inputFile,buffer,BUFFSIZE))>0){
			if(write(fdOutput, buffer, bytesread)<=0){
				printf("ERROR %s\n", strerror(errno));
				return -1;
			}
		}
		i++;
		close(inputFile);
	}

	close(fdOutput);
	
	return (0);
}