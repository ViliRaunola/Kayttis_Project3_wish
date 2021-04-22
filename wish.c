/* Project 3: Unix Shell */
/* wish.c */
/* Jesse Pasanen 0545937 */
/* Vili Raunola 0543366 */

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
#define MAX_ARGS 255
#define PATH_LEN 255
#define MAX_PATHS 20
#define EXIT_CALL "exit"
#define CD_CALL "cd"
#define PATH_CALL "path"

// Error message used for tests. Commented out and replaced with custom ones through out the file
//const char error_message[30] = "An error has occurred\n";

// Source: https://www.javaer101.com/en/article/12327171.html
// used for parsing input string
const char delimiters[] = " \t\r\n\v\f>"; 

void free_arguments(char *arguments[MAX_ARGS]);
void wish_exit(char *arguments[MAX_ARGS], char *line, FILE *input_pointer, char **paths);
void wish_cd(char *arguments[MAX_ARGS], int arg_counter);
void wish_path(char **paths, char **arguments, int arg_counter);
int redirection(char *line, char *argument_line, char *redir_filename);
int create_and_execute_child_process(int redir_flag, char *redir_filename, char **arguments, char *line, char **paths);
void parallel_process_counter(char *line, int *parallel_counter);
void free_paths(char **paths);
void alloc_memory_paths(char **paths);

int main(int argc, char *argv[]){
    /* used in getline() */
    size_t buffer_size = 0; 
    ssize_t line_size;  
    char *line = NULL;

    char default_path[PATH_LEN] = "/bin/";
    char *paths[MAX_PATHS];
    char redir_filename[LEN];
    char *argument_line = NULL;
    char *arg_rest = NULL;
    char *token = NULL;
    char *arguments[MAX_ARGS];
    char *parall_parse_rest = NULL;
    char *parsed_arguments[MAX_ARGS];
    char parsed_line[LEN];
    char *parall_parse_token = NULL;
    int arg_counter = 0;
    int redir_flag = 0;
    int parallel_counter = 0; 
    FILE *input_pointer;

    //Checking if the program was ran with script file
    //If the program was launched without any arguments stdin is given to input pointer
    //If there was an additional argument given then the input pointer will be a pointer to a file from where the reading will be done in the same manner as in normal user input mode.
    if(argc == 1){
        input_pointer = stdin;
    }else if (argc > 2){
        //write(STDERR_FILENO, error_message, strlen(error_message));
        fprintf(stderr, "wish: Too many arguments\n");
        exit(1);
    }else{
        if( (input_pointer = fopen(argv[1], "r")) == NULL ){
            //write(STDERR_FILENO, error_message, strlen(error_message));
            perror("wish: cannot open file");
            exit(1);
        }
    }

    //Creating a char pointer array for paths and assigning default path "/bin/" to the start of the list
    alloc_memory_paths(paths);
    strcpy(paths[0], default_path);
    //Memory slot has to be freed before being assigned to null so it wont be lost.
    free(paths[1]);
    //Assinging NULL so the end of the list is known
    paths[1] = NULL;
    

      
    while(1){
        //Setting flags to 0 at the start of each loop
        redir_flag = 0;
        parallel_counter = 0;

        //Allocating memory for the arguments, char pointer array.
        //All the arguments will be saved here from each read line.
        for(int i = 0; i < MAX_ARGS; i++){
            arguments[i] = malloc(LEN * sizeof(char));
        }	

        //If the program was ran without a file, a text is shown for the user so they know when to give input
        if(argc == 1){
            printf("wish> ");
        }
        
        //How to use getline: man getline
        if( (line_size = getline(&line, &buffer_size, input_pointer)) != -1) {
            //Program ignores the input if only whitespace is given as an input
            if(line_size == 1){
                free_arguments(arguments);
                continue;
            }

            //Removing the newline character from the read line
            line[strlen(line) - 1] = 0;
            
            //If parallel character "&" is found in the given input the program will determine how many of them were given and thus will know how many child processes to create
            if(strstr(line, "&")){

                // calculates the amount of processes
                parallel_process_counter(line, &parallel_counter);

                if (!parallel_counter){
                    free_arguments(arguments);
                    continue;
                }
                if (arguments[0] == NULL) {
                    free_arguments(arguments);
                    continue;
                }

            }else{
                //Assigning the read line to argument_line which will be given to redirection function
                argument_line = line;
                redir_flag = redirection(line, argument_line, redir_filename);
                // redirection() returns 2 if error
                if (redir_flag == 2) {
                    //write(STDERR_FILENO, error_message, strlen(error_message));
                    free_arguments(arguments);
                    continue;
                }

                // arg_rest is used to indicate the rest of the line that is being parsed in the while-loop with strtok_r().
                // First it is the whole line and in the while -loop it is being iterated through
                // Token is always what is before the delimiter and arg_rest is what is after it
                arg_rest = argument_line;
                //arg_counter is used to determine in which position NULL should be assigned in arguments char pointer array
                arg_counter = 0;
                //How to use strtok_r with multiple delimeters source: https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/
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
            }
        } else {
            wish_exit(arguments, line, input_pointer, paths);
        }

        //Checking if the command is a build-in command
        
        //Check first for exit function
        if (!strcmp(arguments[0], EXIT_CALL)){
            //If there are any arguments given to the exit call it will be ignored and error given to the user
            if(arg_counter == 1){
                wish_exit(arguments, line, input_pointer, paths);
            }else{
                //write(STDERR_FILENO, error_message, strlen(error_message));
                fprintf(stderr, "Error: Too many arguments to exit\n");
                free_arguments(arguments);
                continue;
            }
        //Check for build-in cd function
        } else if( !strcmp(arguments[0], CD_CALL) ){
            wish_cd(arguments, arg_counter);
            free_arguments(arguments);
            continue;
        //Check for build-in path function
        } else if (!strcmp(arguments[0], PATH_CALL)) {
            wish_path(paths, arguments, arg_counter);
            free_arguments(arguments);
            continue;
        //If the command wasn't build-in a child process will be created and the command will be executed by execv   
        } else {    
            //How to create multiple parallel child processes. Source: https://stackoverflow.com/questions/876605/multiple-child-process

            // parall_parse_rest is used to indicate the rest of the line that is being parsed in the for-loop with strtok_r().
            // First it is the whole line and in the for -loop it is being iterated through
            parall_parse_rest = line;

            //If there is no need for parallelism, only one child process will be created
            if(parallel_counter < 1){
                if ( !create_and_execute_child_process(redir_flag, redir_filename, arguments, line, paths) ){
                    free_arguments(arguments);
                    continue;
                }
                
                //Have to wait for the child process to finish
                //How to use wait(NULL). Source: https://www.geeksforgeeks.org/wait-system-call-c/
                if ((wait(NULL)) == -1) {	
                    perror("wait");
                }

                free_arguments(arguments);
            //If parallelism is needed, mulitple child processes will be created
            }else{
                //Freeing the arguments char pointer array becuase it is not needed anymore
                free_arguments(arguments);

                //Creating as many child processes as needed
                for(int i = 0; i < parallel_counter; i++){

                    // Creates new char pointer array for arguments between the '&' signs
                    for(int x = 0; x < MAX_ARGS; x++){
                        parsed_arguments[x] = malloc(LEN * sizeof(char));
                    }

                    // Parses the given command and arguments between the '&' signs
                    // In parall_parse_token the command and arguments before each "&" sign is saved
                    parall_parse_token = strtok_r(parall_parse_rest, "&", &parall_parse_rest);
                    strcpy(parsed_line, parall_parse_token);
                    argument_line = parsed_line;

                    // Checks if redirection is needed. Parses the arguments on the left of the ">" and the redirection filename on the right of the ">"
                    redir_flag = redirection(parsed_line, argument_line, redir_filename);
                    // redirection() returns 2 if error
                    if (redir_flag == 2) {
                        //write(STDERR_FILENO, error_message, strlen(error_message));
                        free_arguments(parsed_arguments);
                        continue;
                    }

                    // arg_rest is used to indicate the rest of the line that is being parsed in the while-loop with strtok_r().
                    // First it is the whole line and in the while -loop it is being iterated through
                    // Token is always what is before the delimiter and arg_rest is what is after it
                    arg_rest = argument_line;

                    //Resetting the counter in each loop
                    arg_counter = 0;

                    //Remainder of the line will be split between whitespaces and stored to char pointer array called parsed_arguments which is used to give the command and arguments to execv.
                    //How to use strok_r and with multiple delimeters. Source: https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/
                    while((token = strtok_r(arg_rest, delimiters, &arg_rest))){
                        strcpy(parsed_arguments[arg_counter], token);
                        arg_counter++;
                    }

                    //Memory slot has to be freed before being assigned to null so it wont be lost.
                    free(parsed_arguments[arg_counter]);
                    //Inserting null to the end of the arguments list so execv() will know when to stop reading arguments.
                    parsed_arguments[arg_counter] =  NULL;
                    
                    //If no arguments given with only whitespace on the given line this will be skipped and moved to next command
                    if (parsed_arguments[0] == NULL) {
                        free_arguments(parsed_arguments);
                        continue;
                    }

                    //Creating a child process and giving it the variables to execute the command. line and paths variables are given to it so they can be freed before execv destroys the child process and executes the command
                    if ( !create_and_execute_child_process(redir_flag, redir_filename, parsed_arguments, line, paths) ){
                        free_arguments(parsed_arguments);
                        continue;
                    }
                    free_arguments(parsed_arguments);
                }
                //Waiting for every single child to finish before continuing so that the outputs are printed before the user can continue with wish
                //Source for waiting all the child processes: https://stackoverflow.com/questions/876605/multiple-child-process
                //How to use wait(NULL). Source: https://www.geeksforgeeks.org/wait-system-call-c/
                for(int i = 0; i < parallel_counter; i++){
                    if ((wait(NULL)) == -1) {	
                        perror("wait");
                    }
                }
            }
        }
    }
    return(0);
}

//Used to free a char pointer array that has a lenght of LEN
void free_arguments(char *arguments[MAX_ARGS]){
    for(int i = 0; i < MAX_ARGS; i++){
        free(arguments[i]);
    }
}

//Build-in exit command. Frees all the memory allocations so no leaks would happen when exiting.
//Exit will be given argument 0 to tell that the exit was normal and not due to error 
void wish_exit(char *arguments[LEN], char *line, FILE *input_pointer, char **paths){
    free_arguments(arguments);
    free(line);
    fclose(input_pointer);
    free_paths(paths);
    exit(0);
}

//Build-in cd command to change the current working directory.
//Instructions on how to use chdir() in c: https://www.geeksforgeeks.org/chdir-in-c-language-with-examples/
void wish_cd(char *arguments[LEN], int arg_counter){
    //Check that only one argument is supplied to the cd command
    if(arg_counter != 2){
        //write(STDERR_FILENO, error_message, strlen(error_message));
        fprintf(stderr, "Error: One and only one argument is allowed with the command 'cd'\n");
    }else{
        //If only one command was set for cd the current directory will be changed using chdir().
        //In case of error chdir returns -1
        if(chdir(arguments[1]) == -1){  
            //write(STDERR_FILENO, error_message, strlen(error_message));
            perror("cd");
        }
    }

}

//Frees the char pointer array containing wish's paths
void free_paths(char **paths){
    for(int i = 0; i < MAX_PATHS; i++){
        free(paths[i]);
    }
}

//Allocates memory for wish's paths in a char pointer array
void alloc_memory_paths(char **paths){
    for(int i = 0; i < MAX_PATHS; i++){
        paths[i] = malloc(PATH_LEN * sizeof(char));
    }	
}

//Build-in command to overwrite the default path from which whish will try to run the commands from
void wish_path(char **paths, char **arguments, int arg_counter) {
    int i = 0;

    //Frees and allocates the char pointer array so all the current paths are erased and new ones can be saved
    free_paths(paths);
    alloc_memory_paths(paths);

    //If too many paths are given by the user an error is shown 
    if(arg_counter-1 > MAX_PATHS) {
        fprintf(stderr, "Error: Too many paths given. Max %d\n", MAX_PATHS);
    //If only path was given as argument the path list will be empty
    } else if (arg_counter == 1) {
        //Memory slot has to be freed before being assigned to null so it wont be lost.
        free(paths[0]);
        //Assinging null to the end of the list so the end of the list can be found
        paths[0] = NULL;
    //If more than the command path was given the paths will be saved to char pointer array
    } else if(arg_counter > 1) {
        for(i = 1; i < arg_counter; i++){
            strcpy(paths[i-1], arguments[i]);
            strcat(paths[i-1], "/");
        }
        //Memory slot has to be freed before being assigned to null so it wont be lost.
        free(paths[i-1]);
        //Assinging null to the end of the list so the end of the list can be found
        paths[i-1] = NULL;
    }
}
// Function checks if there are redirection signs in the given input
// If so, parses the input command and arguments from the right side of the ">" and the filename from the left side of the ">"
int redirection(char *line, char *argument_line, char *redir_filename) {
    // source: https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/
    char *filename;
    char *redir_rest = line;
    if(strstr(line, ">")) {
        argument_line = strtok_r(redir_rest, ">", &redir_rest);
        if (strstr(redir_rest, ">")) {
            // too many redirect signs
            fprintf(stderr, "Error: Too many redirection signs\n");
            return 2;
        } 
        //checks if a filename has been given to redirect to
        if ((filename = strtok_r(redir_rest, delimiters, &redir_rest))) {
            if (strtok_r(redir_rest, delimiters, &redir_rest) != NULL) {
                //Too many arguments for redirect
                fprintf(stderr, "Error: Too many arguments after redirection sign\n");
                return 2;
            }
        } else {
            // no namefile given
            fprintf(stderr, "Error: No redirection filename\n");
            return 2;
        }
        strcpy(redir_filename, filename);
        return 1;
    }
    return 0;
}


//Function that creates a child process and in the child process uses execv to run the command
//The switch case structure was implemented from our homework assignment in week 10 task 3.
int create_and_execute_child_process(int redir_flag, char *redir_filename, char **arguments, char *line, char **paths){
    pid_t pid;
    char path[PATH_LEN];
    int valid_paths = 0;

    int i = 0;
    //Testing if the command is found in the defined paths
    while ( paths[i] != NULL ) {
        strcpy(path, paths[i]);
        strcat(path, arguments[0]);
        //If the command is found in one of the paths, the loop can be broken so the current and therefore working path will be used in the execv function
        if( access(path, X_OK) == 0){
            valid_paths = 1;
            break;
        }
        i++;
    }
    //If no paths were found, an error is shown and returned back to main
    if(!valid_paths){
        //write(STDERR_FILENO, error_message, strlen(error_message));
        fprintf(stderr, "Error: command not found in any of the paths\n");
        return(0);
    }
    //The switch case structure was implemented from our homework assignment in week 10 task 3.
    switch (pid = fork()){ //Forking aka creating a new child process
    case -1:
        //write(STDERR_FILENO, error_message, strlen(error_message));
        perror("fork");
        break;
    case 0: //The child prosess
        //If redirection is needed a new file will be created and the output will be written to it
        if(redir_flag){
            //How to use open function with dup2. Source: https://www.youtube.com/watch?v=5fnVr-zH-SE&t=   
            int output_file;

            if( (output_file = open(redir_filename, O_WRONLY | O_CREAT | O_TRUNC , 0777)) == -1){
                //write(STDERR_FILENO, error_message, strlen(error_message));
                fprintf(stderr, "Error: cannot open redirection output file\n");
                free_arguments(arguments);
                free(line);
                exit(1);
            }

            if ((dup2(output_file, STDOUT_FILENO)) == -1) { //Redirect stdout
                perror("dub2");
                free_arguments(arguments);
                free(line);
                exit(1);
            } 
            if ((dup2(output_file, STDERR_FILENO)) == -1) { //Redirect stderr
                perror("dub2");
                free_arguments(arguments);
                free(line);
                exit(1);
            } 
            close(output_file);

            //Error handling for execv
            if (execv(path, arguments) == -1) {
                free_arguments(arguments);
                free(line);
                free_paths(paths);
                //write(STDERR_FILENO, error_message, strlen(error_message));
                perror("execv");
                exit(0);    //Exiting the child when error happens and return back to the parent prosess.
            }

        //If redirection is not needed, only exec will be ran
        } else {
                if (execv(path, arguments) == -1) {
                    free_arguments(arguments);
                    free(line);
                    free_paths(paths);
                    //write(STDERR_FILENO, error_message, strlen(error_message));
                    perror("execv");
                    exit(0);    //Exiting the child when error happens and return back to the parent prosess.
                }
            }
            break;
    default: //Parent process 
        break;
    }

    return(1);
}

    //Counts how many times parallel processes have to be executed
void parallel_process_counter(char *line, int *parallel_counter) {
    // source: https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/
    char parall_temp_line[LEN];
    char *parall_rest, *parall_token;
    // line is copied into a new variable so the pointer doesn't move in the 'line'-variable
    strcpy(parall_temp_line, line);
    *parallel_counter = 0;
    parall_rest = parall_temp_line;
    // parall_rest is used to indicate the rest of the line that is being parsed in the while-loop with strtok_r().
    // First it is the whole line and in the while -loop it is being iterated through
    // parall_token is always what is before the delimiter ("&") and parall_rest is what is after it
    // checks each command and arguments given between "&" signs and calculates the amount
    if((parall_token = strtok_r(parall_rest, "&", &parall_rest))) {
        while(parall_token != NULL) {
            //checks whether or not there is a command after the last "&" sign
            if (strtok(parall_token, delimiters) != NULL) {
                *parallel_counter += 1;
            } else {
                break;
            }
            parall_token = strtok_r(parall_rest, "&", &parall_rest);
        }
    }
}

/***********************EOF***********************/