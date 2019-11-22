/**
 * File: trace.cc
 * ----------------
 * Presents the implementation of the trace program, which traces the execution of another
 * program and prints out information about ever single system call it makes.  For each system call,
 * trace prints:
 *
 *    + the name of the system call,
 *    + the values of all of its arguments, and
 *    + the system calls return value
 */

#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <unistd.h> // for fork, execvp
#include <string.h> // for memchr, strerror
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/wait.h>
#include "trace-options.h"
#include "trace-error-constants.h"
#include "trace-system-calls.h"
#include "trace-exception.h"
using namespace std;

/*
 * systemCallNumbers and systemCallNames store system call names and their number
 * for example, execve's number is 59, systemCallNumbers[59] = "execve"
 * systemCallNames["execve"] = 59
 *
 */
static const int systemCallArgRegs[8] = {RDI, RSI, RDX, R10, R8, R9};
map<int, string> systemCallNumbers;
map<int , string> systemCallErrorStrings;
map<string, int> systemCallNames;
map<string, systemCallSignature> systemCallSignatures;

//delete "\n"
string deleteLineBreak(string notDeleteString){
    string newString;
    while(true){
        if(notDeleteString.find("\n") != string::npos){
            newString = notDeleteString.substr(0,notDeleteString.find("\n"));
            notDeleteString = newString;
        } else
            break;
    }
    return notDeleteString;
}

string getString(pid_t pid ,long address){
    string strings;
    size_t readSize = 0;
    bool judge = true;
    while(judge)
    {
        long longNumber = ptrace(PTRACE_PEEKDATA, pid, address + readSize);
        char * outString = reinterpret_cast<char *>(&longNumber);
        for (int i = 0; i < 8; ++i)
        {
            if(outString[i] == '\0')
            {
                judge = false;
                break;
            }
            strings.push_back(outString[i]);
        }
        readSize += 8;//size of long
    }
    return deleteLineBreak(strings);
}

void fullTraceBefore(const string systemCallName,pid_t pid){
    map<string,systemCallSignature>::iterator iterator;
    vector<scParamType> vector;

    iterator = systemCallSignatures.find(systemCallName);
    if(iterator != systemCallSignatures.end()){
        vector = iterator->second;
        //store the parameter in parentheses.
        int vector_size = vector.size();

        cout << systemCallName << "(";
        for(int i = 0 ; i < vector_size ; i++){
            long longNumber = ptrace(PTRACE_PEEKUSER, pid, systemCallArgRegs[i] * sizeof(long));
            switch(vector[i]){
                case SYSCALL_INTEGER:
                    cout << (int)longNumber;
                    break;
                case SYSCALL_POINTER:
                    if(longNumber != 0)
                        cout << "0x" << std::hex << longNumber << std::dec;//print in hex, according to pdf.
                    else
                        cout << "NULL";//print according to pdf.
                    break;
                case SYSCALL_STRING:
                    cout << "\"" << getString(pid,longNumber) << "\"";
                    break;
                case SYSCALL_UNKNOWN_TYPE:
                    break;
            }//handle different types of signatures.
            if(i != vector_size-1)
                cout << ", ";
        }
        cout << ") = ";
        cout.flush();
    } else{
        cout << systemCallName << "(<signature-information-missing>) = ";
    }
}

void fullTraceAfter(long valueAfter, const string systemCallName){
    if(valueAfter >= 0){
        if(systemCallName == "brk" || systemCallName == "mmap")
            cout << "0x" << std::hex << valueAfter << std::dec;
        else
            cout << (int)valueAfter;
        //print according to pdf.
    }
    else{
        cout << "-1 " << systemCallErrorStrings[-valueAfter]<< " (" << strerror(-(int)valueAfter) << ")";
    }
    cout << endl;
    cout.flush();
}

void simpleTraceBefore(long valueBefore){
    cout << "syscall(" << valueBefore << ") = " << flush;
}

void simpleTraceAfter(long valueAfter){
    cout << (int)valueAfter << endl;
}

/*
 * PTRACE_SYSCALL will let child process run again, but when meeting a system call child will stop again and inform father process.
 */
void trace(int numFlags ,bool simple , char* argv[]){
    pid_t pid = fork();
    if(pid == 0){
        ptrace(PTRACE_TRACEME);
        //child process is traced by its father process
        raise(SIGSTOP);
        //send signal SIGSTOP to child process itself.
        execvp(argv[numFlags + 1],argv + numFlags + 1);
        //call simple-testx.c
    }
    waitpid(pid,NULL,0);
    ptrace(PTRACE_SETOPTIONS,pid,0,PTRACE_O_TRACESYSGOOD);
    while(true){
        string systemCallName;
        while(true){
            int status;
            ptrace(PTRACE_SYSCALL, pid, 0, 0);
            //let child process continue, and stops before a system call.
            waitpid(pid,&status,0);
            if (WIFSTOPPED(status) && (WSTOPSIG(status) & 0x80)){
                long valueBefore = ptrace(PTRACE_PEEKUSER, pid, ORIG_RAX * 8,NULL);//15x8
                //get system call's number, for example, execve is 59.
                systemCallName = systemCallNumbers[valueBefore];
                //get system call's name by its number.
                if(simple){
                    simpleTraceBefore(valueBefore);
                }
                else{
                    fullTraceBefore(systemCallName,pid);
                }
                break;
            }
        }//first while print the contents before "="

        while(true){
            int status;
            ptrace(PTRACE_SYSCALL, pid, 0, 0);
            //let child process run that system call, get its return value.
            waitpid(pid,&status,0);
            if (WIFSTOPPED(status) && (WSTOPSIG(status) & 0x80)){
                long valueAfter = ptrace(PTRACE_PEEKUSER, pid, RAX * 8,NULL);//10x8
                if (simple){
                    simpleTraceAfter(valueAfter);
                }
                else{
                    fullTraceAfter(valueAfter,systemCallName);
                }
                break;
            }
            if (WIFEXITED(status)){
                cout << "<no return>" << endl << "Program exited normally with status " << WEXITSTATUS(status) << endl;
                cout.flush();
                exit(0);
            }
        }//second while print the contents after "=", and control when whole while break.
    }
}

int main(int argc, char *argv[]) {
  // code
    bool simple, rebuild;
    size_t numFlags = processCommandLineFlags(simple,rebuild,argv);
    //if input command has simple , numFlags is 1 , otherwise , 0.
    if(argc - numFlags == 1){
        cout << "Nothing to trace... exiting." << endl;
        return 0;
    }//check value line's input, argc - numFlags = 1 means command is wrong.
    compileSystemCallData(systemCallNumbers,systemCallNames,systemCallSignatures,rebuild);
    compileSystemCallErrorStrings(systemCallErrorStrings);
    //initialize maps
    trace(numFlags,simple,argv);
    //core function
  return 0;
}