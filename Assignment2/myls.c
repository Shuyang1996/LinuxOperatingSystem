#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

void myls (char*, char*, char*, time_t*,int );
void showFileInfo (char*, struct stat *, char*, char*, time_t*, int);
char *uidName( uid_t );
char *gidName( gid_t );

//implement the start program. 
int main(int argc, char* argv[])
{   
    time_t rawtime;
    time( &rawtime );

    int i;
    char name[20]="";
    int mtime;
    //optional parameters, forget these for now. 
    if(argc>1){
        for(i=1; i<argc; i++){
            if(strcmp("-u",argv[i])==0){
                strcpy(name, argv[++i]);
            } else if(strcmp( "-m", argv[i] ) == 0){
                mtime = atoi(argv[++i]);
            } else 
                break;
        }
    } 
    char path[100];
    if(strcmp(" ",argv[i])==0){
        printf("You have to give me a starting path\n");
        return 0;
    } else {
        strcpy(path, argv[i]);
    }
    myls(path,"", name, &rawtime,mtime);
    return 0;
}

void myls(char *dir, char *previous, char*uname, time_t*rawtime, int oldtime)
{

    DIR *dp;
    struct dirent *direntp;
    struct stat statinfo;
    
    if((dp = opendir(dir)) == NULL) {
        fprintf(stderr,"cannot open directory: %s\n", dir);
        return;
    }
    chdir(dir);
    while((direntp = readdir(dp)) != NULL) {
        stat(direntp->d_name,&statinfo);
        
        if(S_ISDIR(statinfo.st_mode)) {
            /* Found a directory, but ignore . and .. */
            if(strcmp(".",direntp->d_name) == 0 || 
                strcmp("..",direntp->d_name) == 0)
                continue;
            char* first  = previous;
            char* second = direntp->d_name;
            char* both = malloc(strlen(first) + strlen(second) + 2);
            strcpy(both, first);
            strcat(both, "/");
            strcat(both, second);
            //need modify
            // showFileInfo(direntp->d_name, &statinfo,previous); please ignore this. 
            myls(direntp->d_name,both,uname,rawtime,oldtime);
        }
        //start from something simple. 
        // else printf("%s/%s\n",previous,direntp->d_name);
        showFileInfo(direntp->d_name, &statinfo, previous, uname,rawtime,oldtime); //two more parameters here one for user id, the other one is group id
    }
    chdir("..");
    closedir(dp);
}

//showFileInfo ()
void showFileInfo(char *filename, struct stat *info_p, char *previous, char *uname,time_t*rawtime, int oldtime){
    //convert mode to letters section
    char modestr[12]; 
    strcpy( modestr, "----------" ); 
    int diff_t;
    diff_t = difftime(*rawtime,info_p->st_mtime);
    //devices detect
    if ( S_ISDIR(info_p->st_mode) )  modestr[0] = 'd';    
    if ( S_ISCHR(info_p->st_mode) )  modestr[0] = 'c';    
    if ( S_ISBLK(info_p->st_mode) )  modestr[0] = 'b';    
    //3 bits for user
    if ( info_p->st_mode & S_IRUSR ) modestr[1] = 'r';    
    if ( info_p->st_mode & S_IWUSR ) modestr[2] = 'w';
    if ( info_p->st_mode & S_IXUSR ) modestr[3] = 'x';
    //3 bits for group
    if ( info_p->st_mode & S_IRGRP ) modestr[4] = 'r';    
    if ( info_p->st_mode & S_IWGRP ) modestr[5] = 'w';
    if ( info_p->st_mode & S_IXGRP ) modestr[6] = 'x';
    //3 bits for other 
    if ( info_p->st_mode & S_IROTH ) modestr[7] = 'r';   
    if ( info_p->st_mode & S_IWOTH ) modestr[8] = 'w';
    if ( info_p->st_mode & S_IXOTH ) modestr[9] = 'x';
    //
    char  *uidName(), *ctime(), *gidName();
    //, *filemode()
    if(((strcmp(uidName(info_p->st_uid),uname)== 0)||(strcmp("",uname)==0)) && (diff_t <= oldtime)){
        
        printf( "%s"    , modestr);
        printf( "%4d "  , info_p->st_nlink); 
        printf( "%-8s " , uidName(info_p->st_uid) );
        printf( "%-8s " , gidName(info_p->st_gid) );
        printf( "%8ld " , (long)info_p->st_size);
        printf( "%.12s", 4+ctime(&info_p->st_mtime));
        printf( "   %s/%s\n"  , previous, filename );
        // printf( "%d\n", diff_t ); //this is the difference of time
    } else {
        printf("");;
    }
}

char *uidName( uid_t uid )
{
    // *getpwuid(),
    struct  passwd  *ptr;
    static  char numstr[10];

    if ( ( ptr = getpwuid( uid ) ) == NULL ){
        sprintf(numstr,"%d", uid);
        return numstr;
    }
    else
        return ptr->pw_name ;
}

char *gidName( gid_t gid )
{
    // *getgrgid(),
    struct group  *ptr;
    static  char numstr[10];

    if ( ( ptr = getgrgid(gid) ) == NULL ){
        sprintf(numstr,"%d", gid);
        return numstr;
    }
    else
        return ptr->gr_name;
}
