#include <sstream>
#include <fstream>

#include "Block.hpp"
#include "output.h"
#include "eval.h"
#include "adapters.h"
#include "SampleMaster.hpp"
#include "Chain.hpp"
#include "TreeRandomizer.hpp"
#include "treeRead.h"
#include "proposals.h"
#include "WrappedProposal.hpp"
#include "ExtendedTBR.hpp"
#include "ExtendedSPR.hpp"
#include "ParsimonySPR.hpp"
#include "StatNNI.hpp"
#include "RadiusMlSPR.hpp"
#include "Category.hpp"
#include "NodeSlider.hpp"


// TODO =( 
#include "GlobalVariables.hpp"

#include "LnlRestorer.hpp"
#include "AvgSplitFreqAssessor.hpp"
#include "ProposalFunctions.hpp"
#include "Parameters.hpp"
#include "TreeLengthMultiplier.hpp"

#include "PartitionProposal.hpp"

extern double masterTime; 

//STAY 
bool SampleMaster::convergenceDiagnostic()
{
  if(numRunConv > 1)    
    { 
      vector<string> fns; 
      for(int i = 0; i < numRunConv; ++i)
	{
	  stringstream ss; 
	  ss <<  PROGRAM_NAME << "_topologies." << runId << "." << i; 
	  fns.push_back(ss.str());
	}
     
      AvgSplitFreqAssessor asdsf(fns);

      int end = asdsf.getEnd();
      
      int treesInBatch = diagFreq / samplingFreq; 

      end /= treesInBatch; 
      end *= treesInBatch;       

      if(end > 0)
	{	  
	  asdsf.setEnd(end);
	  if( burninGen > 0 )
	    {
	      assert(burninProportion == 0.); 

	      int treesToDiscard =  burninGen / samplingFreq; 

	      if(end < treesToDiscard + 2 )
		return false; 
	      else 
		asdsf.setStart(treesToDiscard);  
	    }
	  else 
	    {
	      assert(burninGen == 0); 
	      int start = (int)((double)end * burninProportion  ); 
	      asdsf.setStart(start);
	    } 

	  asdsf.extractBips();
	  double asdsfVal = asdsf.computeAsdsf(asdsfIgnoreFreq);

#if HAVE_PLL == 0      
	  if(processID == 0)
#endif 
	    tout  << "ASDSF or trees " << asdsf.getStart() << "-" << asdsf.getEnd() << ": " <<setprecision(2) << asdsfVal * 100 << "%" << endl; 

	  return asdsfVal < asdsfConvergence; 

	}
      else 
	return false; 
      
    }
  else 
    {
      return runs[0].getChain(0)->currentGeneration > numGen;
    }
}


// LEGACY
static void traverseInitFixedBL(nodeptr p, int *count, TreeAln *traln,  double z )
{
  tree *tr = traln->getTr();
  nodeptr q;
  int i;
  
  for( i = 0; i < traln->getNumBranches(); i++)
      traln->setBranchLengthSave(z, i, p); 
  
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


static void initTreeWithOneRandom(int seed, vector<TreeAln*> &tralns)
{

  TreeAln *traln = tralns[0]; 
  tree *tr = traln->getTr();

  TreeRandomizer trRandomizer(seed, traln);
  trRandomizer.randomizeTree();

  int count = 0; 
  traverseInitFixedBL( tr->start->back, &count, traln, TreeAln::initBL);
  assert(count  == 2 * tr->mxtips - 3);
  
  for(nat i = 1; i < tralns.size(); ++i)
    *(tralns[i])  = *traln;   
}



static void initWithStartingTree(FILE *fh, vector<TreeAln*> &tralns)
{
  // fetch a tree 
  TreeAln *traln = tralns[0]; 
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







void SampleMaster::initWithConfigFile(string configFileName, PriorBelief &prior, vector<double> &proposalWeights )
{
  ConfigReader reader;   
  ifstream fh(configFileName);
  NxsToken token(fh);
  
  ExabayesBlock block(this);
  reader.Add(&block) ; 
  reader.Execute(token);  

  block.fillProposalWeights(proposalWeights);
  validateRunParams();

}


void SampleMaster::validateRunParams()
{
  assert(numCoupledChains > 0); 
  // TODO 
}


SampleMaster::SampleMaster(const CommandLine &cl , ParallelSetup &pl) 
  :  diagFreq(1000)
  , asdsfIgnoreFreq(0.1)
  , asdsfConvergence(0.01)
  , burninGen(5000)
  , burninProportion(0.25)
  , samplingFreq(50)
  , numRunConv(2)
  , numGen(50000)
  , runId(cl.getRunid())
  , numCoupledChains(1)
  , printFreq(500)
  , heatFactor(0.1)
  , swapInterval(1)
  , tuneHeat(true)
  , tuneFreq(50)
  , esprStopProp(0.5)
  , parsimonyWarp(0.1)
  , guidedRadius(5)
{
  FILE *treeFH = NULL; 
  int numTrees = countNumberOfTreesQuick(cl.getTreeFile().c_str()); 

  PriorBelief prior;
  vector<Category> proposals; 

  vector<double> proposalWeihgts; 

  initWithConfigFile(cl.getConfigFileName(), prior, proposalWeihgts);

  
  if( numTrees > 0 )
    treeFH = myfopen(cl.getTreeFile().c_str(), "r"); 

  tout << "Will run " << numRunConv << " runs in total with "<<  numCoupledChains << " coupled chains" << endl; 

#ifdef DEBUG_LNL_VERIFY
  globals.debugTree = new TreeAln();   
  globals.debugTree->initializeFromByteFile(cl.getAlnFileName()); 
  globals.debugTree->enableParsimony();
#endif

  // sets up tree structures 
  vector<TreeAln*>  trees; 
  
  for(int i = 0; i < numCoupledChains; ++i)
    {
      TreeAln *traln = new TreeAln();
      traln->initializeFromByteFile(cl.getAlnFileName()); 
#if HAVE_PLL != 0 
      traln->enableParsimony();
#endif
      trees.push_back(traln); 
    }


  Randomness masterRand(cl.getSeed());   
  vector<int> runSeeds; 
  vector<int> treeSeeds; 
  for(int i = 0; i < numRunConv;++i)
    {
      randCtr_t r = masterRand.generateSeed();
      runSeeds.push_back(r.v[0]); 
      treeSeeds.push_back(r.v[1]); 
    }


  for(int i = 0; i < numRunConv ; ++i)
    {      
      if( i < numTrees)
	initWithStartingTree(treeFH, trees); 
      else 
	initTreeWithOneRandom(treeSeeds[i], trees);

#if HAVE_PLL == 0
      if(i % initParams->numRunParallel != myBatch )
	continue; 
#endif

      runs.push_back(CoupledChains(runSeeds[i], numCoupledChains, trees, i, printFreq, swapInterval, samplingFreq, heatFactor, cl.getRunid(), cl.getWorkdir(), prior, proposals)); 
    }
  
  if(tuneHeat)
    for(auto r : runs)
      r.enableHeatTuning(tuneFreq); 
  
  // only use one restorer for all chains 
  LnlRestorer *restorer = new LnlRestorer(runs[0].getChain(0));
  for(auto run : runs )
    {
      for(int i = 0; i < run.getNumberOfChains(); ++i )
	{
	  Chain *chain = run.getChain(i); 
	  chain->setRestorer(restorer); 
	}
    }

  if(numTrees > 0)
    fclose(treeFH); 

  tout << endl; 
}


void SampleMaster::run()
{
  bool hasConverged = false;   
  while(not hasConverged)   
    {      
      for(nat i = 0; i < runs.size(); ++i)
	{
	  auto run = runs[i]; 
	  run.executePart(diagFreq);
	}

      hasConverged = convergenceDiagnostic(); 

#ifdef ENABLE_PRSF
      if(isOutputProcess())
	printPRSF(run_id);
#endif
    }
}
 
 
void SampleMaster::finalizeRuns()
{
  if(isOutputProcess() )
    {
      for(auto run : runs)
	{
	  for(int i = 0; i < run.getNumberOfChains(); ++i)
	    {
	      Chain *chain = run.getChain(0);
	      if( chain->getChainHeat() == 1.f)
		chain->finalizeOutputFiles(run.getTopoFile());
	    }
	}

      tout << endl << "Converged after " << runs[0].getChain(0)->getGeneration() << " generations" << endl; 
      tout << endl << "Total execution time: " << setprecision(2) <<  gettime() - masterTime <<  " seconds" << endl; 
    }  
}







void SampleMaster::setupProposals(vector<double> proposalWeights, const PriorBelief &prior)
{
  vector<proposalFunction*> legProp; 
  vector<AbstractProposal*> prop; 
  
  // initialize proposals 
  for(int i = 0; i < NUM_PROPOSALS ; ++i)
    { 
      if( NOT isNewProposal[i])
	{
	  proposalFunction *pf = NULL; 
	  initProposalFunction(proposal_type(i), proposalWeights ,&pf); 
	  if(pf != (proposalFunction*) NULL)
	    prop.push_back(new WrappedProposal( pf, NULL)); 
	}
      else 
	{
 	  double weight = proposalWeights[proposal_type(i)]; 
	  if( weight != 0)
	    {
	      switch(proposal_type(i))
		{
		case NODE_SLIDER:
		  prop.push_back(new NodeSlider(NULL, weight, INIT_NODE_SLIDER_MULT, "nodeSlider"));
		  break; 
		case UPDATE_MODEL: 
		  prop.push_back(new PartitionProposal<SlidingProposal, RevMatParameter>(NULL,weight, INIT_RATE_SLID_WIN, "revMatSlider"));
		  break; 
		case FREQUENCY_SLIDER:
		  prop.push_back(new PartitionProposal<SlidingProposal, FrequencyParameter>(NULL, weight, INIT_FREQ_SLID_WIN, "freqSlider"));
		  break; 		  
		case TL_MULT:
		  prop.push_back(new TreeLengthMultiplier(NULL, weight, INIT_TL_MULTI));
		  break; 
		case E_TBR: 
		  prop.push_back(new ExtendedTBR(NULL, weight, esprStopProp, INIT_ESPR_MULT)); 
		  break; 
		case E_SPR: 
		  prop.push_back(new ExtendedSPR(NULL, weight, esprStopProp, INIT_ESPR_MULT)); 
		  break; 
		case PARSIMONY_SPR:	
		  prop.push_back(new ParsimonySPR(NULL, weight, parsimonyWarp, INIT_ESPR_MULT)); 
		  break; 
		case ST_NNI: 
		  prop.push_back(new StatNNI(NULL, weight, INIT_NNI_MULT)); 
		  break; 
		case GAMMA_MULTI: 
		  prop.push_back(new PartitionProposal<MultiplierProposal,RateHetParameter>(NULL, weight, INIT_GAMMA_MULTI, "rateHetMulti")); 
		  break; 
		case UPDATE_GAMMA: 
		  prop.push_back(new PartitionProposal<SlidingProposal,RateHetParameter>(NULL, weight, INIT_GAMMA_SLID_WIN, "rateHetSlider")); 
		  break; 
		case UPDATE_GAMMA_EXP: 
		  prop.push_back(new PartitionProposal<ExponentialProposal,RateHetParameter>(NULL, weight, 0, "rateHetExp")); 
		  break; 
		case UPDATE_FREQUENCIES_DIRICHLET: 
		  prop.push_back(new PartitionProposal<DirichletProposal,FrequencyParameter>(NULL, weight, INIT_DIRICHLET_ALPHA, "freqDirich")); 
		  break; 
		case UPDATE_MODEL_DIRICHLET: 
		  prop.push_back(new PartitionProposal<DirichletProposal,RevMatParameter>(NULL,weight, INIT_DIRICHLET_ALPHA, "revMatDirich"));
		  break; 
		case GUIDED_SPR:
		  prop.push_back(new RadiusMlSPR(NULL, weight, guidedRadius )); 
		default : 
		  assert(0); 
		}
	    }
	}  
    }


  // danger,  we cannot rely on the fact that all proposals are used 
  assert(0); 


  // get total sum 
  double sum = 0; 
  for(int i = 0; i < NUM_PROPOSALS; ++i)    
    sum += proposalWeights[i]; 

  vector<Category> proposalCategories; 

  // create categories 
  vector<string> allNames = {"", "Topology", "BranchLengths", "Frequencies", "RevMatrix", "RateHet" }; 
  for(int i = 1; i < NUM_PROP_CATS+1; ++i)
    {
      // fish out the correct proposals 
      vector<proposalFunction*> legPr; 
      vector<AbstractProposal*> pr; 
      double catSum = 0; 
      for(auto p : legProp)
	if(p->category == i)
	  {
	    legPr.push_back(p); 
	    catSum += p->relativeWeight; 
	  }
      for(auto p : prop)
	if(p->getCategory() == i)
	  {
	    pr.push_back(p); 
	    catSum += p->getRelativeProbability();
	  }

      if(pr.size() > 0 || legPr.size() > 0)
	proposalCategories.push_back( Category(allNames[i], category_t(i), catSum / sum, pr )); 
    }    


  if ( isOutputProcess() )
    {
      // print some info 
      tout << "using the following moves: " << endl; 
      for(nat i = 0; i < proposalCategories.size(); ++i)
	{
	  // TODO also to info file 
      
	  tout << proposalCategories[i].getName() << " " << fixed << setprecision(2) << proposalCategories[i].getCatFreq() * 100 << "%\t" ; 
	  auto cat = proposalCategories[i]; 
	  auto p1 =  cat.getProposals(); 
	  for(auto p : p1)
	    tout << "\t" << p->getName() << "(" << fixed << setprecision(2) <<  p->getRelativeProbability() * 100 << "%)" ;
	  tout << endl; 
	}
    }
}
