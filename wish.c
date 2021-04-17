/* Source: https://stackoverflow.com/questions/59014090/warning-implicit-declaration-of-function-getline */
/* error with getline() solved with '#define  _POSIX_C_SOURCE 200809L' */
#define  _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#define LEN 250

const char error_message[30] = "An error has occurred\n";

int main(int argc, char *argv[]){
    pid_t pid;
    char *line = NULL;
    size_t buffer_size = 0;
    char *temp;
    char *commands[LEN];
    int i, status;
    ssize_t line_size;

    //printf("Hello World!\n");
  
    while(1){

        for(int i = 0; i < LEN; i++){
            commands[i] = malloc(LEN);
        }	

        printf("wish> ");
        if( (line_size = getline(&line, &buffer_size, stdin)) != -1) {
            //Program continues if only empty line is passed to it.
            if(line_size == 1){
                continue;
            }
            line[strlen(line) - 1] = 0;
            temp = strtok(line, " ");
            i = 0;
            while(temp != NULL){
                strcpy(commands[i], temp);
                i++;
                temp = strtok(NULL, " ");
            }
            
            commands[i] = NULL;
        }else{
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
     
        char path[LEN] = "/bin/";
        strcat(path, commands[0]);        

        switch (pid = fork())
        {
        case -1:
            write(STDERR_FILENO, error_message, strlen(error_message));
            break;
        case 0: //The child prosess
            if (execv(path, commands) == -1) {
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
            break;
        default: //Parent process

            if (wait(&status) == -1) {	//Odottaa lapsen päättymistä
                    perror("wait");
                    exit(1);
            }

            break;
        }
        

    for(int i = 0; i < LEN; i++){
        free(commands[i]);
    }

    }
    free(line);

    for(int i = 0; i < LEN; i++){
        free(commands[i]);
    }

    return(0);
}