//
// Created by 夏禹天 on 2019/11/16.
//
#include "subprocess.h"
#include <iostream>
using namespace std;

void createPipe(int fds[2]){
    if(pipe(fds) == -1)
        throw SubprocessException("Encountered a problem while creating pipes.");
}

void callFork(int pid){
    if(pid == -1)
        throw SubprocessException("Encountered a problem while creating a subprocess");
}

void callClose(int fd){
    if(close(fd) == -1)
        throw SubprocessException("Encountered a problem while closing a pipe's write port");
}

void callDup2(int fd1, int fd2){
    if(dup2(fd1,fd2) == -1)
        throw SubprocessException("Encountered a problem while connecting pipe with standard input");
}
void callExecvp(char* argv[]){
    if(execvp(argv[0],argv) == -1)
        throw SubprocessException("Encountered a problem while calling execvp function");
}
subprocess_t subprocess(char *argv[], bool supplyChildInput, bool ingestChildOutput) throw (SubprocessException){
    int supply_fds[2];
    int ingest_fds[2];
    /* i use two pipes to implement this part, supply pipe's 0 port is associated with standard input,
     * ingest pipe's 1 port is associated with standard output.
     * outside test program use supply pipe's 1 port to write contents into pipe and ingest
     * pipe's 0 port to read contents.
     */
    struct subprocess_t subprocess = {0,kNotInUse,kNotInUse};
    //initialize
    try{
        createPipe(supply_fds);
        createPipe(ingest_fds);
        //throw exception while creating pipes wrongly.

        subprocess = {fork(),kNotInUse,kNotInUse};

        callFork(subprocess.pid);

        if(subprocess.pid == 0){
            //subprocess's task
            callClose(supply_fds[1]);
            if(supplyChildInput){
                callDup2(supply_fds[0],STDIN_FILENO);
            }
            callClose(supply_fds[0]);
            callClose(ingest_fds[0]);
            if(ingestChildOutput){
                callDup2(ingest_fds[1],STDOUT_FILENO);
            }
            callClose(ingest_fds[1]);
            callExecvp(argv);
        }
        close(supply_fds[0]);
        close(ingest_fds[1]);
        if(supplyChildInput)
            subprocess.supplyfd = supply_fds[1];
        if(ingestChildOutput)
            subprocess.ingestfd = ingest_fds[0];
    }catch (const SubprocessException& se){
        cerr << "More details here: " << se.what() << endl;
    }catch (...){
        cerr << "Unknown internal error." << endl;
    }
    return subprocess;
}
