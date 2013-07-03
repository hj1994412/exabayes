/** 
    @file exabayes.cpp
    
    @brief This file sets the flavour of exabayes that has been
    compiled (i.e., sequential, pll, examl, ...)
    
*/ 



// TODO re-activate all that initial bla bla when starting up
// exa-bayes (model / program info )


#ifdef HAVE_AVX
#define __AVX
#endif

#include <sstream>

#include "axml.h" 

#define _INCLUDE_DEFINITIONS
#include "GlobalVariables.hpp"
#undef _INCLUDE_DEFINITIONS

#include "time.hpp"

#include "config/CommandLine.hpp"
#include "SampleMaster.hpp"
#include "ParallelSetup.hpp"

#include "teestream.hpp"

// #define TEST  

#ifdef TEST
#include "TreeRandomizer.hpp"
#include "Chain.hpp"
#include "BranchLengthMultiplier.hpp"
#include "ParsimonyEvaluator.hpp"
#endif


// have ae look at that later again 
double fastPow(double a, double b) {
  union {
    double d;
    int x[2];
  } u = { a };
  u.x[1] = (int)(b * (u.x[1] - 1072632447) + 1072632447);
  u.x[0] = 0;
  return u.d;
}


#include <chrono>


/**
   @brief the main ExaBayes function.

  @param tr -- a tree structure that has been initialize in one of the adapter mains. 
   @param adef -- the legacy adef
 */
static void exa_main (const CommandLine &cl, const ParallelSetup &pl )
{   
  timeIncrement = CLOCK::system_clock::now(); 

#ifdef TEST     

  auto t =  make_shared<TreeAln>( 1,2)  ; 
  t->initializeFromByteFile(cl.getAlnFileName());
  t->enableParsimony();

  randCtr_t c; 
  c.v[1] = 0; 
  c.v[0] = 1; 

  TreeRandomizer r(c); 
  r.randomizeTree(*t);
  
  vector<nat> partPars ; 
  ParsimonyEvaluator p; 
  p.evaluate(*t, t->getTr()->start, true,partPars);
  
  for(auto &v : partPars)
    cout << v << "," ; 
  cout << endl; 

#else 

  SampleMaster master(  pl, cl );
  master.initializeRuns(); 
  master.run();
  master.finalizeRuns();
#endif
}


/* 
   tell the CPU to ignore exceptions generated by denormalized floating point values.
   If this is not done, depending on the input data, the likelihood functions can exhibit 
   substantial run-time differences for vectors of equal length.
*/

void ignoreExceptionsDenormFloat()
{
#if ! (defined(__ppc) || defined(__powerpc__) || defined(PPC))
  _mm_setcsr( _mm_getcsr() | _MM_FLUSH_ZERO_ON);
#endif   
}


#if HAVE_PLL != 0
#include "globalVariables.h"
#endif

#if HAVE_PLL == 0 
extern int processID; 
extern int processes;
extern MPI_Comm comm; 
#endif


void makeInfoFile(const CommandLine &cl, const ParallelSetup &pl )
{
  stringstream ss; 
  string workdir =  cl.getWorkdir(); 
  ss << workdir ; 
  if(workdir.compare("") != 0 )
    ss << "/" ; 
  ss << PROGRAM_NAME << "_info."  << cl.getRunid() ;

  // TODO maybe check for existance 

  globals.logFile = ss.str();   
  
  globals.logStream =  new ofstream  (globals.logFile); 
  globals.teeOut =  new teestream(cout, *globals.logStream);

  if(not pl.isReportingProcess())
    tout.disable(); 
}


void initializeProfiler()
{
  // see this page for info 
  // http://google-perftools.googlecode.com/svn/trunk/doc/cpuprofile.html  
  // that option is important
  // CPUPROFILE_FREQUENCY=x
#ifdef _USE_GOOGLE_PROFILER
  ProfilerStart("profile.out");
#endif
}
 

 
void finalizeProfiler()
{
#ifdef _USE_GOOGLE_PROFILER
  ProfilerStop();
#endif
}


int main(int argc, char **argv)
{ 
  ParallelSetup pl(argc,argv); 		// MUST be the first thing to do because of mpi_init ! 

  initializeProfiler();

#if HAVE_PLL != 0 && ( (defined(_FINE_GRAIN_MPI) || defined(_USE_PTHREADS)))
  assert(0); 
#endif

  ignoreExceptionsDenormFloat(); 
  CommandLine cl(argc, argv); 

#if HAVE_PLL == 0 
  pl.initializeExaml(cl);
#endif

  makeInfoFile(cl, pl);

  // cl.printVersion(true);  
  tout << endl; 

  tout << PROGRAM_NAME << " was called as follows: " << endl; 
  for(int i = 0; i < argc; ++i)
    tout << argv[i] << " " ; 
  tout << endl << endl; 

  exa_main( cl, pl); 

  finalizeProfiler();
  pl.finalize();  
  
  delete globals.logStream; 
  delete globals.teeOut;
  return 0;
}
