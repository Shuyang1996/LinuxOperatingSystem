#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

void sig_handler(int signal);
int questionA();
int questionB();
int questionC();
int questionD(off_t size);
int questionE(off_t size);
int questionF();

int main (int argc, char *argv[], char *env[]){
	//signal handler
	if(signal(SIGINT,sig_handler)== SIG_ERR){
		fprintf(stderr,"Signal handler failed: %s \n",strerror(errno));
	}

	if(signal(SIGBUS,sig_handler)== SIG_ERR){
		fprintf(stderr,"Signal handler failed: %s \n",strerror(errno));
	}

	if(signal(SIGILL,sig_handler)== SIG_ERR){
		fprintf(stderr,"Signal handler failed: %s \n",strerror(errno));
	}

	if(signal(SIGSEGV,sig_handler)== SIG_ERR){
		fprintf(stderr,"Signal handler failed: %s \n",strerror(errno));
	}

	struct stat buf;
	char file[100];
	strcpy(file,argv[1]);
	

	if(stat("test.txt", &buf )<0 ){
		fprintf(stderr, "fstat function failed: %s\n", strerror(errno) );
		exit(1);
	}
	off_t size = buf.st_size;

	if(argc==2){
		switch(argv[1][0]-'A'+1){
			case 1:
				return questionA();
				
			case 2:
				return questionB(MAP_SHARED);
				
			case 3:
				return questionC();
				//the size of this file is not changed. Unless it is appended when opened and written
				//It is a different part of the memory.

			case 4:
				return questionD(size);
				//the values between the end of the old file and where the fd is now with values 
				//in the mapping. Thus values in the whole becomes a value in the real file. 
			case 5:
				return questionE(size); 
				//the mapped file is filled with 0 in the first page, and file only exists 
				//in the first page. Thus, terminal returns SIGBUS.
			case 6:
				return questionF();
				
			default:
				printf("You have options from A to F\n");
				break;
		}
	}

	return 0;
}

int questionA(){
	//the answer for question is SIGSEGV
	printf("Answer for question A\n");
	int fd;
	char *text = "shuyang's homework";
	fd = open("test.txt",O_RDWR);
	if(fd==-1){
		fprintf(stderr, "Cannot open file test.txt %s\n", strerror(errno));
		return -1;
	}
	char *pmap = mmap(0,4096,PROT_READ,MAP_SHARED,fd, 0);

	for(int i=0; i<19; i++){
		pmap[i]=text[i];
	}
	return 0;
}

int questionB(){
	printf("Answer for question question B\n");
	char *pmap;
	int fd = open("test.txt",O_RDWR);
	
	if(fd == -1){
		fprintf(stderr, "Cannot open test.txt %s\n", strerror(errno) );
		return -1;
	}

	pmap = mmap(0,4096,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	
	if(pmap == MAP_FAILED){
		fprintf(stderr, "Mmap failed :%s\n", strerror(errno) );
		return -1;
	}

	close(fd);

	sprintf(pmap,"shuyang's homework");

	int fd2 = open("test.txt",O_RDONLY);
	
	if(fd2 == -1){
		fprintf(stderr, "Cannot open test.txt %s\n", strerror(errno) );
		return -1;
	} 

	char buffer[1024];
	int bytesread = read(fd2, buffer,1024);
	if(bytesread == -1){
		fprintf(stderr, "Read function failed %s\n", strerror(errno) );
		return -1;
	}

	if(strcmp(buffer,"shuyang's homework")!=0){
		printf("The answer to this question is No.\n");
	}else{
		printf("The answer to this question is Yes.\n");
	}

	close(fd2);

	return 0;
}

int questionC(){
	printf("Answer for question question C\n");
	char *pmap;
	int fd = open("test.txt",O_RDWR);
	
	if(fd == -1){
		fprintf(stderr, "Cannot open test.txt %s\n", strerror(errno) );
		return -1;
	}

	pmap = mmap(0,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE,fd,0);
	
	if(pmap == MAP_FAILED){
		fprintf(stderr, "Mmap failed :%s\n", strerror(errno) );
		return -1;
	}

	close(fd);

	sprintf(pmap,"shuyang's homework");

	int fd2 = open("test.txt",O_RDONLY);
	
	if(fd2 == -1){
		fprintf(stderr, "Cannot open test.txt %s\n", strerror(errno) );
		return -1;
	} 

	char buffer[1024];
	int bytesread = read(fd2, buffer,1024);
	if(bytesread == -1){
		fprintf(stderr, "Read function failed %s\n", strerror(errno) );
		return -1;
	}

	if(strcmp(buffer,"shuyang's homework")!=0){
		printf("The answer to this question is YES.\n");
	}else{
		printf("The answer to this question is NO.\n");
	}

	close(fd2);

	return 0;
	
}

int questionD(off_t size){
	printf("The answer to question D\n");
	printf("The original size of test.txt is %zu \n",size);
	struct stat file;
	int fd;
	fd = open("test.txt", O_RDWR);
	if(fd == -1){
		fprintf(stderr, "Cannot open test.txt %s\n", strerror(errno) );
		return -1;
	}
	char *pmap;
	pmap = mmap(0,4096,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);

	if(pmap == MAP_FAILED){
		fprintf(stderr, "Mmap failed :%s\n", strerror(errno) );
		return -1;
	}

	for(off_t x = 0; x<size; x++){
		pmap++;
	}

	char *c = pmap;
	*c = 'Y';
	if(stat("test.txt",&file)<0){
		fprintf(stderr, "Stat function failed %s\n", strerror(errno) );
		return -1;
	}
	printf("Wrote one bit 'Y' beyond the file size.\n");
	printf("The size of the file now %zu\n", file.st_size );
	if(file.st_size == size){
		printf("No, the size does not change\n");
	}else{
		printf("Yes, the size does change.\n");
	}
	return 0;
}

int questionE(off_t size){
	struct stat file;
	int fd;
	fd = open("test.txt", O_RDWR);
	if(fd == -1){
		fprintf(stderr, "Cannot open test.txt %s\n", strerror(errno) );
		return -1;
	}
	char *pmap;
	pmap = mmap(0,4096,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);

	if(pmap == MAP_FAILED){
		fprintf(stderr, "Mmap failed :%s\n", strerror(errno) );
		return -1;
	}
	off_t x=0;
	for(; x<size; x++){
		pmap++;
	}

	char *c = pmap;
	*c = 'Y';
	if(stat("test.txt",&file)<0){
		fprintf(stderr, "Stat function failed %s\n", strerror(errno) );
		return -1;
	}

	if(lseek(fd,x+1,SEEK_CUR)<0){
		fprintf(stderr, "Lseek function failed %s\n",strerror(errno) );
		return -1;
	}
	if(write(fd,"Shuyang",7)<0){
		fprintf(stderr, "write function failed %s\n",strerror(errno) );
		return -1;
	}
	off_t hole = lseek(fd,-8, SEEK_CUR);
	if(hole<0){
		fprintf(stderr, "Lseek function failed %s\n", strerror(errno));
		return -1;
	}

	char buffer[1];
	if(read(fd,buffer,1)<0){
		fprintf(stderr, "Cannot read from the file %s\n", strerror(errno));
		return -1;
	}
	if (*c == buffer[0]){
		printf("Data writen in the hole remain visible \n");
	}else{
		printf("Data written in the hole is not visible \n");
	}
	return 0;
}

int questionF(){
	printf("Answer for question F\n");
	printf("First, create small file test2.txt of size 10 bytes\n");
	FILE *fd = fopen("test2.txt","w");
	
	fprintf(fd, "shuyang" );
	fclose(fd);
	int fd2 = open("test2.txt",O_RDWR);

	if(fd2 == -1){
		fprintf(stderr, "Cannot open test2.txt %s\n", strerror(errno) );
		return -1;
	}
	char *pmap = mmap(0,8192,PROT_WRITE|PROT_READ,MAP_SHARED,fd2,0);
	if(pmap == MAP_FAILED){
		fprintf(stderr, "Mmap failed :%s\n", strerror(errno) );
		return -1;
	}
	int page;
	close(fd2);
	printf("Enter the page you want to like at first one or second one?\n");
	scanf("%d",&page);
	page =page*4000;
	char *c = pmap;
	printf("%c\n",c[page]);
	//if you choose page 1 no signal will be generated. 
	printf("no signal generated\n");
	return 0;
}

void sig_handler(int signal){
	if(signal == SIGSEGV){
		printf("SIGSEGV signal received.\n");
	}
	else if(signal == SIGBUS){
		printf("SIGBUS signal received.\n");
	}
	else if(signal == SIGILL){
		printf("SIGILL signal received\n");
	}
	else if(signal == SIGINT){
		printf("SIGINT signal received\n");
	}
	exit(1);
}









