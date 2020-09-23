#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_LINE 80 // max length command
#define IN_DIRECTION 2
#define OUT_DIRECTION 1
#define NO_DIRECTION 0
#define READ_END 0
#define WRITE_END 1


int main(void){
    char *args[MAX_LINE/2 + 1];   // command line arguments
    char *args2[MAX_LINE/2 + 1];  // second program after '|'
    int should_run = 1;
    const char *delim = " \t\r\n\v\f";
    char *cmd = (char*)malloc(MAX_LINE);
    char *last_cmd = NULL;
    pid_t pid = -1;

    for (int i = 0; i < MAX_LINE/2 + 1; ++i){   
        args[i] = NULL;
        args2[i] = NULL;
    }

    while (should_run){
        printf("osh>");
        fflush(stdout);

      
        // Read command
        int arg_num = 0;    // number of arguments    
        if (fgets(cmd, MAX_LINE, stdin) == NULL) {
            fprintf(stderr, "Error reading command.\n");
            free(last_cmd);
            free(cmd);
            exit(1); 
        }

        // Look for !! (last command)
        if (cmd[0] == '!' && cmd[1] == '!' && 
            (cmd[2] == ' ' || cmd[2] == '\t' || cmd[2] == '\r' 
             || cmd[2] == '\n' || cmd[2] == '\v' || cmd[2] == '\f'))
        {
            if (last_cmd == NULL){
                printf("No command in history\n");
                continue;
            }
            printf("Last command: %s", last_cmd);
            snprintf(cmd, MAX_LINE, "%s", last_cmd);
        } 
        else {   // update command history    
            if (last_cmd == NULL)   last_cmd = (char*)malloc(MAX_LINE);    
            snprintf(last_cmd, MAX_LINE, "%s", cmd);
        }       

        // break command into tokens         
        args[arg_num] = strtok(cmd, delim);     // get 1st token   
        while (args[arg_num] != NULL){
            ++arg_num;
            args[arg_num] = strtok(NULL, delim);
        }

        // if user enters nothing
        if (arg_num == 0) continue;

        // Look for exit, quit
        if (strcmp(args[0], "quit") == 0 || strcmp(args[0], "exit") == 0){
            should_run = 0;
            break;
        }

        // Look for & (background process)
        int bg_flag = 0;
        if (strcmp(args[arg_num-1], "&") == 0){
            bg_flag = 1;
            args[arg_num-1] = NULL;     // remove '&' in command
            --arg_num;
        }

        // Look for > (out direction), < (in direction)
        int direction = NO_DIRECTION;   
        int f_index;            // index of file_name 
        for (f_index = 0;  f_index < arg_num; ++f_index){
            if (strcmp(args[f_index], "<") == 0) {
                direction = IN_DIRECTION;
                break;
            }
            else if (strcmp(args[f_index], ">") == 0) {
                direction = OUT_DIRECTION;
                break;
            }
        }
        if (direction != NO_DIRECTION){
            // f_index was at location of '<' or '>', so increase it by 1
            f_index++; 
        }
        
        // Look for pipe
        int pipef = 0;
        int p_index = 0;
        for (p_index = 0; p_index < arg_num; ++p_index) {
            if (strcmp(args[p_index], "|") == 0){
                pipef = 1;
                break;
            }
        }
        if (pipef) {  // split into 2 programs
            int j = 0; // arguments count for second program
            for (int i = p_index+1; i < arg_num; ++i) {
                args2[j] = args[i];
                ++j;
            }
            args2[j] = NULL;
            args[p_index] = NULL;
        }


        // After reading user input, the steps are:
        // * (1) fork a child process using fork()
        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Fork failed.");
            return 1;
        }
        // * (2) the child process will invoke execvp()
        else if (pid == 0){
            
            // if pipe detected
            if (pipef) {
                int pfd[2];
                pid_t ppid = -1;  // pipe pid

                if (pipe(pfd) == -1){
                    fprintf(stderr, "Pipe failed.\n");
                    exit(1);
                }
             
                ppid = fork();
                if (ppid < 0) {  
                    fprintf(stderr, "Fork failed.\n");
                    exit(1);
                }
                if (ppid == 0) {    // first program
                    close(pfd[READ_END]);
                    dup2(pfd[WRITE_END], STDOUT_FILENO);
                    close(pfd[WRITE_END]);

                    if (execvp(args[0], args) < 0) {
                        printf("Error executing command.\n");
                        return 1;
                    }
                }
                else if (ppid > 0) {  // second program
                    close(pfd[WRITE_END]);
                    dup2(pfd[READ_END], STDIN_FILENO);
                    close(pfd[READ_END]);

                    wait(NULL); // wait for input from first program
                    if (execvp(args2[0], args2) < 0) {
                        printf("Error executing command.\n");
                        return 1;
                    }
                }
            }

            // if cmd has redirection
            else if (direction != NO_DIRECTION) {
                FILE *fd;    // file descriptor

                if (direction == OUT_DIRECTION){
                    fd = fopen(args[f_index], "w");
                    dup2(fileno(fd), STDOUT_FILENO);
                }
                else if (direction == IN_DIRECTION){
                    fd = fopen(args[f_index], "r");
                    if (fd == NULL){
                        printf("%s not found.\n", args[f_index]);
                        return 1;
                    }
                    dup2(fileno(fd), STDIN_FILENO);
                }

                fclose(fd);
                args[f_index-1] = NULL; // get the cmd before '<' or '>' to execute
            }

            if (execvp(args[0], args) < 0) {
                printf("Error executing command.\n");
                return 1;
            }
        }
        // * (3) parent will invoke wait() unless command included &
        else{
            if (bg_flag == 0) {
                waitpid(pid, NULL, 0);
                // printf("Child complete.\n");
            }
            else {
                printf("pid %d running.\n", pid);
            }
        }
    }

    if (last_cmd != NULL)
        free(last_cmd);
    free(cmd);
    return 0;
}
