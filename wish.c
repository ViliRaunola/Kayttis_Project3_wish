/* Source: https://stackoverflow.com/questions/59014090/warning-implicit-declaration-of-function-getline */
/* error with getline() solved with '#define  _POSIX_C_SOURCE 200809L' */
#define  _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#define LEN 255
#define EXIT_CALL "exit"

const char error_message[30] = "An error has occurred\n";

void free_arguments(char *arguments[LEN]);
void wish_exit(char **arguments, char *line);

int main(int argc, char *argv[]){
    pid_t pid;
    size_t buffer_size = 0;
    ssize_t line_size;
    char *line = NULL;
    char *temp;
    char *arguments[LEN];
    int i, status;
      
    while(1){

        for(int i = 0; i < LEN; i++){
            arguments[i] = malloc(LEN * sizeof(char));
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
                strcpy(arguments[i], temp);
                i++;
                temp = strtok(NULL, " ");
            }

            //Inserting null to the end of the arguments list so 
            free(arguments[i]);
            arguments[i] =  NULL;
        
        }else{
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }

        //Tähän kohtaa tarkistetaan onko oma vai systeemi kutsu
        wish_exit(arguments, line);

        char path[LEN] = "/bin/";
        strcat(path, arguments[0]);        

        //The switch case structure was implemented from our homework assignment in week 10 task 3.
        switch (pid = fork()){
        case -1:
            write(STDERR_FILENO, error_message, strlen(error_message));
            break;
        case 0: //The child prosess
            if (execv(path, arguments) == -1) {
                free_arguments(arguments);
                free(line);
                write(STDERR_FILENO, error_message, strlen(error_message));
                exit(0);    //Exiting the child when error happens and return back to the parent prosess.
            }
            
            break;
        default: //Parent process

            if (wait(&status) == -1) {	//Odottaa lapsen päättymistä
                    perror("wait");
                    exit(1);
            }

            break;
        }

        
    free_arguments(arguments);
    }
    

    return(0);
}

void free_arguments(char *arguments[LEN]){
    for(int i = 0; i < LEN; i++){
        free(arguments[i]);
    }
}


void wish_exit(char *arguments[LEN], char *line){
    if (!strcmp(arguments[0], EXIT_CALL)) {
        free_arguments(arguments);
        free(line);
        exit(0);
    }
    
}