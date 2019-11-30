//
// Created by 夏禹天 on 2019/11/18.
//
#include <cassert>
#include <ctime>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>
#include <sched.h>
#include "subprocess.h"
#include <signal.h>
using namespace std;

struct worker {
    worker() {}
    worker(char *argv[]) : sp(subprocess(argv, true, false)),
                           available(false) {}
    subprocess_t sp;
    bool available;
};
static const size_t kNumCPUs = sysconf(_SC_NPROCESSORS_ONLN);
static vector<worker> workers(kNumCPUs); // space for kNumCPUs, zero-arg constructed workers
static size_t numWorkersAvailable;
static const char *kWorkerArguments[] = {"./factor.py", "--self-halting", NULL};

static size_t getAvailableWorker(){
    sigset_t new_sig_set, old_sig_set;
    sigemptyset(&new_sig_set);
    sigaddset(&new_sig_set, SIGCHLD);
    sigprocmask(SIG_BLOCK, &new_sig_set, &old_sig_set);
    while(numWorkersAvailable == 0)
    {
        sigsuspend(&old_sig_set);//wait for the signal SIGCHILD, if get it, continue program, otherwise, wait until get one.
    }
    size_t worker_available;
    for (size_t i = 0; i < kNumCPUs; i++)
    {
        if (workers[i].available)
        {
            worker_available = i;
            workers[i].available = false;
            numWorkersAvailable--;
            break;
        }
    }
    sigprocmask(SIG_UNBLOCK, &new_sig_set, NULL);
    return worker_available;
}

static void spawnAllWorkers(){
    cout << "There are this many CPUs: " << kNumCPUs << ", numbered 0 through " << kNumCPUs-1 << "." << endl;
    for(size_t i = 0 ; i < kNumCPUs ; i++){
        cpu_set_t cpu_set;
        CPU_ZERO(&cpu_set);//clear the set
        CPU_SET(i,&cpu_set);//add cpu to cpu_set
        workers[i] = worker(const_cast<char**>(kWorkerArguments));//constructor,create a subprocess.
        sched_setaffinity(workers[i].sp.pid, sizeof(cpu_set_t),&cpu_set);//assign a process to always execute on a particular core
        cout << "Worker " << workers[i].sp.pid << " is set to run on CPU " << i << "." << endl;
    }
}
/*
 * when a process self-halt, call this function.
 * By calling waitpid to make sure the subprocess has self-halted.
 * After above process, mark it as a available worker.
 */
static void markWorkersAsAvailable(int signal){
    while(true){
        pid_t pid = waitpid(-1,NULL,WNOHANG|WUNTRACED);
        if (pid <= 0)
            break;
        for (size_t i = 0; i < kNumCPUs; i++){
            if (workers[i].sp.pid == pid){
                workers[i].available = true;
                numWorkersAvailable++;
                break;
            }
        }
    }
}

static void broadcastNumbersToWorkers(){
    while (true) {
        string line;
        getline(cin, line);
        if (cin.fail()) break;
        size_t endpos;
        /* long long num = */ stoll(line, &endpos);
        if (endpos != line.size()) break;
        size_t worker_available_index = getAvailableWorker();
        write(workers[worker_available_index].sp.supplyfd, line.c_str(), line.size());
        write(workers[worker_available_index].sp.supplyfd, "\n", 1);
        kill(workers[worker_available_index].sp.pid, SIGCONT);
        // you shouldn't need all that many lines of additional code
    }
}

static void waitForAllWorkers(){
    sigset_t new_sig_set,existing;
    sigemptyset(&new_sig_set);
    sigaddset(&new_sig_set, SIGCHLD);
    sigprocmask(SIG_BLOCK, &new_sig_set, &existing);
    while (numWorkersAvailable != kNumCPUs) {
        sigsuspend(&existing);
    }
    sigprocmask(SIG_UNBLOCK, &new_sig_set, NULL);
}

static void closeAllWorkers(){
    signal(SIGCHLD, SIG_DFL);
    for (size_t i = 0; i < kNumCPUs; i++) {
        close(workers[i].sp.supplyfd);
        kill(workers[i].sp.pid, SIGCONT);
    }
    for(size_t i = 0 ; i < kNumCPUs ; i++){
        waitpid(-1,NULL,0);
    }
}

int main(int argc, char *argv[]) {
    numWorkersAvailable = 0;//initialize available worker's number
    signal(SIGCHLD, markWorkersAsAvailable);//if subprocess's status changed, do this.
    spawnAllWorkers();
    broadcastNumbersToWorkers();
    waitForAllWorkers();
    closeAllWorkers();
    return 0;
}