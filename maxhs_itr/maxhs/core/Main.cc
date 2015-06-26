/***********[Main.cc]
Copyright (c) 2012-2013 Jessica Davies, Fahiem Bacchus

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

***********/

#include <errno.h>
#include <signal.h>
#include <iostream>

#include "minisat/utils/System.h"
#include "minisat/utils/Options.h"
#include "maxhs/core/MaxSolver.h"
#include "maxhs/core/Wcnf.h"
#include "maxhs/utils/Params.h"
using std::cout;

static MaxHS::MaxSolver* thesolver {};

static void SIGINT_exit(int signum) {
    if (thesolver) {
        thesolver->printStatsAndExit(signum, 1);
    } else {
        fflush(stdout);
        fflush(stderr);
        // Note that '_exit()' rather than 'exit()' has to be used.
        // The reason is that 'exit()' calls destructors and may cause deadlocks 
        // if a malloc/free function happens to be running (these functions are guarded by 
        //  locks for multithreaded use).
        _exit(0);
    }
}

int main(int argc, char** argv) {
  try {
    setUsageHelp("USAGE: %s [options] <input-file>\n  where input may be either in plain or gzipped DIMACS.\n");
    
#if defined(__linux__)
    fpu_control_t oldcw, newcw;
    _FPU_GETCW(oldcw); newcw = (oldcw & ~_FPU_EXTENDED) | _FPU_DOUBLE; _FPU_SETCW(newcw);
#endif
    
    IntOption    cpu_lim("MAIN", "cpu-lim","Limit on CPU time allowed in seconds.\n", INT32_MAX, IntRange(0, INT32_MAX));
    IntOption    mem_lim("MAIN", "mem-lim","Limit on memory usage in megabytes.\n", INT32_MAX, IntRange(0, INT32_MAX));
    BoolOption   version("MAIN", "version", "Print version number and exit", false);
    
    parseOptions(argc, argv, true);
    params.readOptions();

    if(version) {
      printf("MaxHS %d.%d\n", 2, 51);
      return(0);
    }
    printf("c MaxHS %d.%d\n", 2, 51);
    
    signal(SIGINT, SIGINT_exit);
    signal(SIGXCPU, SIGINT_exit);
    signal(SIGSEGV, SIGINT_exit);
    signal(SIGTERM, SIGINT_exit);
    signal(SIGABRT, SIGINT_exit);
    
    if (cpu_lim != INT32_MAX){
      rlimit rl;
      getrlimit(RLIMIT_CPU, &rl);
      if (rl.rlim_max == RLIM_INFINITY || (rlim_t)cpu_lim < rl.rlim_max){
	rl.rlim_cur = cpu_lim;
	if (setrlimit(RLIMIT_CPU, &rl) == -1)
	  printf("c WARNING! Could not set resource limit: CPU-time.\n");
      } }
    
    if (mem_lim != INT32_MAX){
      rlim_t new_mem_lim = (rlim_t)mem_lim * 1024*1024;
      rlimit rl;
      getrlimit(RLIMIT_AS, &rl);
      if (rl.rlim_max == RLIM_INFINITY || new_mem_lim < rl.rlim_max){
	rl.rlim_cur = new_mem_lim;
	if (setrlimit(RLIMIT_AS, &rl) == -1)
	  printf("c WARNING! Could not set resource limit: Virtual memory.\n");
      } }
    
    if(argc < 2) {
      cout << "c ERROR: no input file specfied:\n"
	"USAGE: %s [options] <input-file>\n  where input may be either in plain or gzipped DIMACS.\n";
      exit(0);
    }
    
    bool flag=false;
    if (argc>2)
      if (argv[2]=="-f"){
        flag=true;
        cout<<"flag -f\n";
      }

    

    Wcnf theFormula {};
    if (!theFormula.inputDimacs(argv[1])) 
      return 1;
    theFormula.printFormulaStats();

    MaxHS::MaxSolver S(&theFormula);
    thesolver = &S;
    if(flag) S.solvewithcores(argv[3]);
    else S.solve_maxsat();
    S.printStatsAndExit(-1, 0);
  }
  catch(std::bad_alloc) {
    cout << "c Memory Exceeded\n";
    thesolver->printStatsAndExit(100, 1);
  }
  catch (...) {
    printf("c Unknown exception probably memory.\n");
    thesolver->printStatsAndExit(200, 1);
  } 
  fflush(stdout);
  fflush(stderr);
  return 0;
}
