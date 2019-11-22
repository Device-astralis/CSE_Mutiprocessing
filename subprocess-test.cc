/**
 * File: subprocess-test.cc
 * ------------------------
 * Simple unit test framework in place to exercise functionality of the subprocess function.
 */

#include "subprocess.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sys/wait.h>
#include <ext/stdio_filebuf.h>

using namespace __gnu_cxx; // __gnu_cxx::stdio_filebuf -> stdio_filebuf
using namespace std;

/**
 * File: publishWordsToChild
 * -------------------------
 * Algorithmically self-explanatory.  Relies on a g++ extension where iostreams can
 * be wrapped around file desriptors so that we can use operator<<, getline, endl, etc.
 */
const string kWords[] = {"put", "a", "ring", "on", "it"};
static void publishWordsToChild(int to) {
  stdio_filebuf<char> outbuf(to, std::ios::out);
  ostream os(&outbuf); // manufacture an ostream out of a write-oriented file descriptor so we can use C++ streams semantics (prettier!)
  for (const string& word: kWords) {
      os << "supply: " << word << endl;
  }
} // stdio_filebuf destroyed, destructor calls close on desciptor it owns

/**
 * File: ingestAndPublishWords
 * ---------------------------
 * Reads in everything from the provided file descriptor, which should be
 * the sorted content that the child process running /usr/bin/sort publishes to
 * its standard out.  Note that we one again rely on the same g++ extenstion that
 * allows us to wrap an iostream around a file descriptor so we have C++ stream semantics
 * available to us.
 */
static void ingestAndPublishWords(int from) {
  stdio_filebuf<char> inbuf(from, std::ios::in);
  istream is(&inbuf);
  while (true) {
    string word;
    getline(is, word);
    if (is.fail()) break;
    cout << "ingest: " << word << endl;
  }
} // stdio_filebuf destroyed, destructor calls close on desciptor it owns

/**
 * Function: waitForChildProcess
 * -----------------------------
 * Halts execution until the process with the provided id exits.
 */
static void waitForChildProcess(pid_t pid) {
  if (waitpid(pid, NULL, 0) != pid) {
    throw SubprocessException("Encountered a problem while waiting for subprocess's process to finish.");
  }
}

/**
 * Function: main
 * --------------
 * Serves as the entry point for for the unit test.
 */
const string kSortExecutable = "sort";
const string kDateExecutable = "date";

void test_true_and_false(bool supplyChildInput , bool ingestChildOutput , int serial , const string kExecutable){
    cout << "test" << serial << ":" << endl;
    char *argv[] = {const_cast<char *>(kExecutable.c_str()), NULL};
    subprocess_t child = subprocess(argv, supplyChildInput, ingestChildOutput);
    publishWordsToChild(child.supplyfd);
    ingestAndPublishWords(child.ingestfd);
    waitForChildProcess(child.pid);
}
int main(int argc, char *argv[]) {
  try {
    test_true_and_false(true,true,1,kSortExecutable);
    test_true_and_false(true,false,2,kSortExecutable);
    //first two test functions have output.
    test_true_and_false(false,true,3,kDateExecutable);
    test_true_and_false(false,false,4,kDateExecutable);
    /* last two test functions don't have output, only have date output.
     * since nothing is wrote into supply's pipe(i guess, since i don't clearly know what stdio_filebuf has done)
     * therefore, we can't use sort to sort contents in stdin, if we really do this, program will be in
     * an unresponsive state.
     * All in all, ingestAnd... function doesn't print anything means ingest's 0 read port reads nothing at all.
     * Date's print is just a record, delete its call is definitely ok.
     * The reason why test3's output has 'ingest:' is that stdout is associated with child's ingest's write port.
     * */
    return 0;
  } catch (const SubprocessException& se) {
    cerr << "Problem encountered while spawning second process to run \"" << kSortExecutable << "\"." << endl;
    cerr << "More details here: " << se.what() << endl;
    return 1;
  } catch (...) { // ... here means catch everything else
    cerr << "Unknown internal error." << endl;
    return 2;
  }
}