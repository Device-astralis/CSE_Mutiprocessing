//
// Created by 夏禹天 on 2019/11/16.
//
#include <unistd.h>
#include <stdio.h>

void chage(pid_t pids[]){
    pids[0] = 1;
    pids[1] = 2;
}

int main(){
    pid_t pids[2];
    printf("pid1 %d\n",pids[0]);
    printf("pid2 %d\n",pids[1]);
    chage(pids);
    printf("pids1 %d\n",pids[0]);
    printf("pids2 %d\n",pids[1]);
    return 0;
}
