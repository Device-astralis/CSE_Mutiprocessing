//
// Created by 夏禹天 on 2019/11/15.
//
#include "pipeline.h"
#include <sys/wait.h>

void pipeline(char *argv1[], char *argv2[], pid_t pids[]){
    int fds[2];
    pipe(fds);
    //construct a pipe
    int read_fd = fds[0];
    int write_fd = fds[1];
    //get read and write's file descriptor
    pid_t child_process_one = fork();
    pids[0] = child_process_one;
    //get child process's pid
    if (child_process_one == 0) {
        close(read_fd);
        dup2(write_fd, STDOUT_FILENO);
        //connect write operation with standard output in child process one
        close(write_fd);
        execvp(argv1[0], argv1);
    }

    pid_t child_process_two = fork();
    pids[1] = child_process_two;

    if (child_process_two == 0) {
        close(write_fd);
        dup2(read_fd, STDIN_FILENO);
        //connect read operation with standard input in child process two
        close(read_fd);
        execvp(argv2[0], argv2);
    }
    close(read_fd);
    close(write_fd);
    //close read and write file descriptor
}
