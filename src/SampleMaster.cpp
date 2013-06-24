#include <sstream>
#include <fstream>
#include <memory>

#include "config/BlockProposalConfig.hpp"
#include "config/BlockRunParameters.hpp"
#include "config/ConfigReader.hpp"

#include "SampleMaster.hpp"
#include "Chain.hpp"
#include "TreeRandomizer.hpp"
#include "treeRead.h"
#include "tune.h"
#include "RunFactory.hpp"	

#include "GlobalVariables.hpp"

#include "LnlRestorer.hpp"
#include "AvgSplitFreqAssessor.hpp"
#include "ProposalFunctions.hpp"
#include "Parameters.hpp"

extern double masterTime; 

static int countNumberOfTreesQuick(const char *fn ); 
static void initWithStartingTree(FILE *fh, vector<shared_ptr<TreeAln> > tralns); 
static void initTreeWithOneRandom(int seed, vector<shared_ptr<TreeAln> > tralns); 

SampleMaster::SampleMaster(const ParallelSetup &pl) 
  : pl(pl)
  , initTime(gettime())
{

}



void SampleMaster::initializeRuns(const CommandLine &cl )
{
  FILE *treeFH = NULL; 
  int numTreesAvailable = countNumberOfTreesQuick(cl.getTreeFile().c_str()); 

  // initialize one tree 
  vector<TreeAlnPtr > trees; 
  trees.push_back(TreeAlnPtr(new TreeAln()));
  trees[0]->initializeFromByteFile(cl.getAlnFileName()); 
  trees[0]->enableParsimony();

  vector<ProposalPtr> proposals; 
  vector<RandomVariablePtr> variables; 

  initWithConfigFile(cl.getConfigFileName(), trees[0], proposals, variables);
  assert(runParams.getTuneFreq() > 0); 

  // ORDER: must be after  initWithConfigFile
  initTrees(trees, cl);

  if( numTreesAvailable > 0 )
    treeFH = myfopen(cl.getTreeFile().c_str(), "r"); 
  
  Randomness masterRand(cl.getSeed());   
  vector<int> runSeeds; 
  vector<int> treeSeeds; 
  for(int i = 0; i < runParams.getNumRunConv();++i)
    {
      randCtr_t r = masterRand.generateSeed();
      runSeeds.push_back(r.v[0]); 
      treeSeeds.push_back(r.v[1]); 
    }


  LnlRestorerPtr restorer(new LnlRestorer(*(trees[0])));
  LikelihoodEvaluatorPtr eval(new LikelihoodEvaluator(restorer));

  for(int i = 0; i < runParams.getNumRunConv() ; ++i)
    {      
      if( i < numTreesAvailable)
	initWithStartingTree(treeFH, trees); 
      else 
	initTreeWithOneRandom(treeSeeds[i], trees);

      if( i %  pl.getRunsParallel() == pl.getMyRunBatch() )
	runs.push_back(CoupledChains(runSeeds[i], i, runParams, trees, cl.getWorkdir(), proposals, variables, eval)); 
    }

  if(runParams.getTuneHeat())
    for(CoupledChains& r : runs)
      r.enableHeatTuning(runParams.getTuneFreq()); 

  if(numTreesAvailable > 0)
    fclose(treeFH); 

  tout << endl; 
}


//STAY 
bool SampleMaster::convergenceDiagnostic()
{
  if(runParams.getNumRunConv() > 1)    
    { 
      vector<string> fns; 
      for(int i = 0; i < runParams.getNumRunConv(); ++i)
	{
	  stringstream ss; 
	  ss <<  PROGRAM_NAME << "_topologies." << runParams.getRunId() << "." << i; 
	  fns.push_back(ss.str());
	}
     
      AvgSplitFreqAssessor asdsf(fns);

      int end = asdsf.getEnd();
      
      int treesInBatch = runParams.getDiagFreq() / runParams.getSamplingFreq(); 

      end /= treesInBatch; 
      end *= treesInBatch;       

      if(end > 0)
	{	  
	  asdsf.setEnd(end);
	  if( runParams.getBurninGen() > 0 )
	    {
	      assert(runParams.getBurninProportion() == 0.); 

	      int treesToDiscard =  runParams.getBurninGen() / runParams.getSamplingFreq(); 

	      if(end < treesToDiscard + 2 )
		return false; 
	      else 
		asdsf.setStart(treesToDiscard);  
	    }
	  else 
	    {
	      assert(runParams.getBurninGen() == 0); 
	      int start = (int)((double)end * runParams.getBurninProportion()  ); 
	      asdsf.setStart(start);
	    } 

	  asdsf.extractBips();
	  double asdsfVal = asdsf.computeAsdsf(runParams.getAsdsfIgnoreFreq());

#if HAVE_PLL == 0      
	  if(processID == 0)
#endif 
	    tout  << "ASDSF for trees " << asdsf.getStart() << "-" << asdsf.getEnd() << ": " <<setprecision(2) << asdsfVal * 100 << "%" << endl; 

	  return asdsfVal < runParams.getAsdsfConvergence(); 

	}
      else 
	return false; 
      
    }
  else 
    {
      return runs[0].getChain(0)->getGeneration() > runParams.getNumGen();
    }
}


// LEGACY
static void traverseInitFixedBL(nodeptr p, int *count, shared_ptr<TreeAln> traln,  double z )
{
  tree *tr = traln->getTr();
  nodeptr q;
  int i;
  
  for( i = 0; i < traln->getNumBranches(); i++)
      traln->setBranchLengthBounded(z, i, p); 
  
  *count += 1;
  
  if (! isTip(p->number,tr->mxtips)) 
    {                                  /*  Adjust descendants */
      q = p->next;
      while (q != p) 
	{
	  traverseInitFixedBL(q->back, count, traln, z);
	  q = q->next;
	} 
    }
}


static void initTreeWithOneRandom(int seed, vector<shared_ptr<TreeAln> > tralns)
{

  auto traln = tralns[0]; 
  tree *tr = traln->getTr();

  TreeRandomizer trRandomizer(seed, traln);
  trRandomizer.randomizeTree();

  int count = 0; 
  traverseInitFixedBL( tr->start->back, &count, traln, TreeAln::initBL);
  assert(count  == 2 * tr->mxtips - 3);
  
  for(nat i = 1; i < tralns.size(); ++i)
    *(tralns[i])  = *traln;   
}



static void initWithStartingTree(FILE *fh, vector<shared_ptr<TreeAln> > tralns)
{
  // fetch a tree 
  auto traln = tralns[0]; 
  tree *tr = traln->getTr();
  boolean hasBranchLength =  readTreeWithOrWithoutBL(tr, fh);

  int count = 0;       
  if(hasBranchLength)
    traverseInitCorrect(tr->start->back, &count, traln ) ;  
  else      
    traverseInitFixedBL(tr->start->back, &count, traln, TreeAln::initBL ); 

  assert(count == 2 * tr->mxtips  -3);       

  for(nat i = 1 ; i < tralns.size(); ++i)
    *(tralns[i]) = *traln; 
}


// TODO finalize output files  

static int countNumberOfTreesQuick(const char *fn )
{
  FILE *fh = fopen(fn, "r"); 

  if(fh == 0)
    return 0; 

  int c = 0; 
  int result = 0; 
  while( ( c = getc(fh) ) != EOF)
    {
      if(c == ';')
	++result; 	
    }
  
  fclose(fh);
  return result; 
}


void SampleMaster::initWithConfigFile(string configFileName, TreeAlnPtr traln, vector<ProposalPtr> &proposalResult, vector<RandomVariablePtr> &variableResult)
{
  ConfigReader reader; 
  ifstream fh(configFileName); 
  NxsToken token(fh); 

  paramBlock.setTree(traln); 
  reader.Add(&paramBlock); 

  BlockPrior priorBlock(traln->getNumberOfPartitions());
  reader.Add(&priorBlock);

  reader.Add(&runParams);

  BlockProposalConfig proposalConfig; 
  reader.Add(&proposalConfig);   

  reader.Execute(token);

  RunFactory r; 
  r.configureRuns(proposalConfig, priorBlock , paramBlock, *traln, proposalResult);
  variableResult = r.getRandomVariables(); 
}


void SampleMaster::validateRunParams()
{
  assert(runParams.getNumCoupledChains() > 0); 
  // TODO 
}



void SampleMaster::initTrees(vector<TreeAlnPtr> &trees, const CommandLine &cl )
{  

  tout << endl << "Will run " << runParams.getNumRunConv() << " runs in total with "<<  runParams.getNumCoupledChains() << " coupled chains" << endl << endl; 
#ifdef DEBUG_LNL_VERIFY
  globals.debugTree = new TreeAln();   
  globals.debugTree->initializeFromByteFile(cl.getAlnFileName()); 
  globals.debugTree->enableParsimony();
#endif

  for(int i = trees.size(); i < runParams.getNumCoupledChains(); ++i)
    {
      auto traln =  TreeAlnPtr(new TreeAln());
      traln->initializeFromByteFile(cl.getAlnFileName()); 
      traln->enableParsimony();
      trees.push_back(traln); 
    }

}


void SampleMaster::run()
{
  bool hasConverged = false;   
  while(not hasConverged)   
    {      
      for(nat i = 0; i < runs.size(); ++i)
	{
	  auto run = runs[i]; 
	  run.executePart(runParams.getDiagFreq());
	}

      hasConverged = convergenceDiagnostic(); 

#ifdef ENABLE_PRSF      
      printPRSF(run_id);
#endif
    }
}
 
 
void SampleMaster::finalizeRuns()
{
  for(auto run : runs)
    {
      for(int i = 0; i < run.getNumberOfChains(); ++i)
	{
	  Chain *chain = run.getChain(0);
	  if( chain->getChainHeat() == 1.f)
	    chain->finalizeOutputFiles(run.getTopoFile());
	  tout << "best state was: " << chain->getBestState( )<< endl; 
	}
    }

  tout << endl << "Converged after " << runs[0].getChain(0)->getGeneration() << " generations" << endl; 
  tout << endl << "Total execution time: " << setprecision(2) <<  gettime() - initTime <<  " seconds" << endl; 
}

