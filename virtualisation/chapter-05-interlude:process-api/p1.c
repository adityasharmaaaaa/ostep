#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]){
    printf("hello world (pid:%d)\n",(int)getpid());
    int rc = fork();
    if(rc < 0){
        fprintf(stderr,"fork failed\n");  //fork failed; exit
        exit(1);
    }
    else if(rc == 0){ //child (new proces)
        printf("hello i am child (pid:%d)\n",(int)getpid());   
    }
    else{ //parent goes down this path (main)
        printf("hello i am parent of %d (pid%d)\n", rc,(int)getpid());
    }
    return 0;
}