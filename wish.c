/* Source: https://stackoverflow.com/questions/59014090/warning-implicit-declaration-of-function-getline */
/* error with getline() solved with '#define  _POSIX_C_SOURCE 200809L' */
#define  _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>      
#define LEN 255
#define PATH_LEN 255
#define EXIT_CALL "exit"
#define CD_CALL "cd"
#define PATH_CALL "path"

const char error_message[30] = "An error has occurred\n";

void free_arguments(char *arguments[LEN]);
void wish_exit(char *arguments[LEN], char *line, FILE *input_pointer);
void wish_cd(char *arguments[LEN], int arg_counter);
int wish_path(char *default_path, char **arguments, int no_args);
int redirection(char *line, char *argument_line, char **arguments, char *delimiters, char *redir_filename, char *redir_rest);

int main(int argc, char *argv[]){
    pid_t pid;
    size_t buffer_size = 0;
    ssize_t line_size;
    char *line = NULL;
    char *token = NULL;
    char *arguments[LEN];
    char default_path[PATH_LEN] = "/bin";
    char path[PATH_LEN];
    char redir_filename[LEN];
    char delimiters[] = " \t\r\n\v\f>"; //Source: https://www.javaer101.com/en/article/12327171.html
    char *argument_line = NULL;
    char *redir_rest = NULL;
    char *arg_rest = NULL;
    int arg_counter, status;
    int redir_flag = 0;
    FILE *input_pointer;

    if(argc == 1){
        input_pointer = stdin;
    }else if (argc > 2){
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }else{
        if( (input_pointer = fopen(argv[1], "r")) == NULL ){
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
    }
    
      
    while(1){
        redir_flag = 0;
        for(int i = 0; i < LEN; i++){
            arguments[i] = malloc(LEN * sizeof(char));
        }	

        if(argc == 1){
            printf("wish> ");
        }
        
        if( (line_size = getline(&line, &buffer_size, input_pointer)) != -1) {
            //Program continues if only empty line is passed to it.
            if(line_size == 1){
                continue;
            }
            //Remove new line character from input
            line[strlen(line) - 1] = 0;
            argument_line = line;
            redir_rest = line;
            redir_flag = redirection(line, argument_line, arguments, delimiters, redir_filename, redir_rest);
            // redir_flag returns 2 if error
            if (redir_flag == 2) {
                write(STDERR_FILENO, error_message, strlen(error_message));
                free_arguments(arguments);
                continue;
            }

            arg_rest = argument_line;
            arg_counter = 0;
            while((token = strtok_r(arg_rest, delimiters, &arg_rest))){
                strcpy(arguments[arg_counter], token);
                arg_counter++;
            }

        //Memory slot has to be freed before being assigned to null so it wont be lost.
        free(arguments[arg_counter]);
        //Inserting null to the end of the arguments list so execv() will know when to stop reading arguments.
        arguments[arg_counter] =  NULL;
        
        //if no argument given with only whitespace on the given line
        if (arguments[0] == NULL) {
            free_arguments(arguments);
            continue;
        }

        } else {
                wish_exit(arguments, line, input_pointer);
        }

        //Tähän kohtaa tarkistetaan onko oma vai systeemi kutsu
        
        if (!strcmp(arguments[0], EXIT_CALL)){
            if(arg_counter == 1){
                wish_exit(arguments, line, input_pointer);
            }else{
                write(STDERR_FILENO, error_message, strlen(error_message));
                continue;
            }
        } else if( !strcmp(arguments[0], CD_CALL) ){
            wish_cd(arguments, arg_counter);
        } else if (!strcmp(arguments[0], PATH_CALL)) {
            if(wish_path(default_path, arguments, arg_counter)) {
                continue;
            } 
        } else {       
            strcpy(path, default_path);
            strcat(path, "/");
            strcat(path, arguments[0]);
            //The switch case structure was implemented from our homework assignment in week 10 task 3.
            switch (pid = fork()){
            case -1:
                write(STDERR_FILENO, error_message, strlen(error_message));
                break;
            case 0: //The child prosess

                if(redir_flag){

                    //How to use open function with dup2. Source: https://www.youtube.com/watch?v=5fnVr-zH-SE&t=   
                    int output_file;

                    if( (output_file = open(redir_filename, O_WRONLY | O_CREAT, 0777)) == -1){
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        free_arguments(arguments);
                        free(line);
                        exit(1);
                    }

                    dup2(output_file, STDOUT_FILENO); //Redirect stdout
                    dup2(output_file, STDERR_FILENO); //Redirect stderr
                    close(output_file);

                    if (execv(path, arguments) == -1) {
                        free_arguments(arguments);
                        free(line);
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        exit(0);    //Exiting the child when error happens and return back to the parent prosess.
                    }

                } else {
                        if (execv(path, arguments) == -1) {
                            free_arguments(arguments);
                            free(line);
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            exit(0);    //Exiting the child when error happens and return back to the parent prosess.
                        }
                    }
                    break;
            default: //Parent process

                if (wait(&status) == -1) {	//Odottaa lapsen päättymistä
                        perror("wait");
                        exit(1);
                }
                break;
            }
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


void wish_exit(char *arguments[LEN], char *line, FILE *input_pointer){
    free_arguments(arguments);
    free(line);
    fclose(input_pointer);
    exit(0);
}

//Instructions on how to use chdir() in c: https://www.geeksforgeeks.org/chdir-in-c-language-with-examples/
void wish_cd(char *arguments[LEN], int arg_counter){
    //Check that only one argument is supplied to the cd command
    if(arg_counter != 2){
        write(STDERR_FILENO, error_message, strlen(error_message));
    }else{
        if(chdir(arguments[1]) == -1){  //If only one command was set for cd the current directory will be changed using chdir()
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
    }

}


int wish_path(char *default_path, char **arguments, int arg_counter) {
    if (arg_counter == 1) {
        strcpy(default_path, "");
    } else if(arg_counter > 1) {
        // luo lista patheja
        strcpy(default_path, arguments[1]);
        strcat(default_path, "/");
        return 1;
    }
    return 0;
}

int redirection(char *line, char *argument_line, char **arguments, char *delimiters, char *redir_filename, char *redir_rest) {
    // source: https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/
    char *filename;
    if(strstr(line, ">")) {
        argument_line = strtok_r(redir_rest, ">", &redir_rest);
        if (strstr(redir_rest, ">")) {
            // too many redirect signs
            return 2;
        } 
        //checks if a filename has been given to redirect to
        if ((filename = strtok_r(redir_rest, delimiters, &redir_rest))) {
            if (strtok_r(redir_rest, delimiters, &redir_rest) != NULL) {
                //Too many arguments for redirect
                return 2;
            }
        } else {
            // no file given
            return 2;
        }
        strcpy(redir_filename, filename);
        return 1;
    }
    return 0;
}