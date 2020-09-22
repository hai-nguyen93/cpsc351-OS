#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_LINE 80 // max length command

int main(void){

    char *args[MAX_LINE/2 + 1]; // command line arguments
    int should_run = 1;
    const char *delim = " \t\r\n\v\f";
    char *cmd;
    char *last_cmd = NULL;
    pid_t pid;

    while (should_run){
        printf("osh>");
        fflush(stdout);
      
        // Read command
        int arg_num = 0;    // number of arguments    
        if (fgets(cmd, MAX_LINE, stdin) == NULL) {
            fprintf(stderr, "Error reading command.\n");
            free(last_cmd);
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
            strcpy(cmd, last_cmd);
        } 
        else {   // update command history    
            if (last_cmd == NULL)   last_cmd = (char*)malloc(MAX_LINE);    
            strcpy(last_cmd, cmd);
        }       

        // break command into tokens         
        args[arg_num] = strtok(cmd, delim);     // get 1st token   
        while (args[arg_num] != NULL){
            ++arg_num;
            args[arg_num] = strtok(NULL, delim);
        }

        // // print tokens
        // printf("Number of arguments: %d\n", arg_num);
        // for(int i = 0; i < arg_num; ++i){
        //     printf("args[%d]: %s\n", i, args[i]);
        // }
        // printf("-------\n");

        if (arg_num == 0) continue;

        // Look for exit, quit
        if (strcmp(args[0], "quit") == 0 || strcmp(args[0], "exit") == 0){
            should_run = 0;
            continue;
        }

        // Look for & (background process)
        int bg_flag = 0;
        if (strcmp(args[arg_num-1], "&") == 0){
            bg_flag = 1;
            args[arg_num-1] = NULL;     // remove '&' in command
            --arg_num;
        }


        // After reading user input, the steps are:
        // * (1) fork a child process using fork()
        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Fork failed.");
            return 1;
        }
        else if (pid == 0){
        // * (2) the child process will invoke execvp()
            execvp(args[0], args);
        }
        else{
        // * (3) parent will invoke wait() unless command included &
            if (bg_flag == 0) {
                waitpid(pid, NULL, 0);
                printf("Child complete.\n");
            }
            else {
                printf("pid %d running.\n", pid);
            }
        }
    }
    
    free(last_cmd);
    return 0;
}