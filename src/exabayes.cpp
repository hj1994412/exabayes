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

#include "axml.h" 

#define _INCLUDE_DEFINITIONS
#include "GlobalVariables.hpp"
#undef _INCLUDE_DEFINITIONS

#include "tune.h"
#include "output.h"
#include "adapters.h"
#include "CommandLine.hpp"
#include "SampleMaster.hpp"
#include "ParallelSetup.hpp"

#include "teestream.hpp"

// #define TEST  
// #include "branch.h"
// #include "TreeRandomizer.hpp"

/**
   @brief the main ExaBayes function.

  @param tr -- a tree structure that has been initialize in one of the adapter mains. 
   @param adef -- the legacy adef
 */
void exa_main (const CommandLine &cl, ParallelSetup &pl )
{   
  timeIncrement = gettime();

#ifdef TEST   

#if 0 
  TreeAln traln; 
  traln.initializeFromByteFile(byteFileName);
  traln.enableParsimony(); 

  TreeRandomizer r(123, &traln); 
  r.randomizeTree();
  tree *tr = traln.getTr(); 
  
  for(int i = tr->mxtips+1; i < 2 * tr->mxtips; ++i)
    {
      cout << exa_evaluateParsimony(traln, tr->nodep[i], TRUE ) << endl; 
    }
#endif


  TreeAln traln; 
  traln.initializeFromByteFile(cl.getAlnFileName()); 
  TreeRandomizer r(123, &traln); 
  r.randomizeTree();

  vector<branch> result; 
  extractBranches(traln, result);

  cout << "got "<< result.size() << " branches" << endl; 
  
  for(auto b : result)
    cout << b  << endl; 

  exit(0);

#endif

  SampleMaster master( cl, pl);
  master.run();
  master.finalizeRuns();
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




using namespace std; 

#include <sstream>
void makeInfoFile(const CommandLine &cl)
{
  stringstream ss; 
  string workdir =  cl.getWorkdir(); 
  ss << workdir ; 
  if(workdir.compare("") != 0 )
    ss << "/" ; 
  ss << PROGRAM_NAME << "_info."  << cl.getRunid() ;
  
  // TODO maybe check for existance 

  globals.logFile = ss.str();   
  globals.logStream = new ofstream (globals.logFile) ; 
  globals.teeOut = new teestream(cout, *globals.logStream);
}



int main(int argc, char *argv[])
{   
  ParallelSetup pl(argc,argv); 		// MUST be the first thing to do because of mpi_init ! 


#if HAVE_PLL != 0 && ( (defined(_FINE_GRAIN_MPI) || defined(_USE_PTHREADS)))
  assert(0); 
#endif

  ignoreExceptionsDenormFloat(); 
  CommandLine cl(argc, argv); 

  makeInfoFile(cl);

  cl.printVersion(true);  
  tout << endl; 

  tout << PROGRAM_NAME << " was called as follows: " << endl; 
  for(int i = 0; i < argc; ++i)
    tout << argv[i] << " " ; 
  tout << endl << endl; 



#if HAVE_PLL == 0 
  pl.initializeExaml(cl);
#endif


  exa_main( cl, pl); 

  pl.finalize();  
  return 0;
}
