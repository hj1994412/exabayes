

#include "LnlRestorer.hpp" 
#include "adapters.h"
#include "branch.h"
#include "TreeAln.hpp" 
#include "globals.h"

#include <iostream>
using namespace std; 




LnlRestorer::LnlRestorer(state *_chain)
  : chain(_chain)
{
  tree *tr = chain->traln->getTr(); 
  int numPart = chain->traln->getNumberOfPartitions(),
    numTax = tr->mxtips; 
  
  reserveArrays = (double***)exa_calloc(numPart, sizeof(double**)); 
  partitionScaler = (nat**)exa_calloc(numPart, sizeof(nat*)); 
  
  for(int i = 0; i < numPart; ++i)
    {
      pInfo *partition = chain->traln->getPartition(i );
      reserveArrays[i] = (double**)exa_calloc(tr->mxtips, sizeof(double*)); 

#if HAVE_PLL != 0		/* TODO that's hacky */
      int length = partition->upper - partition->lower; 
#else 
      int length = partition->width; 
#endif

      for(int j = 0; j < numTax-2; ++j)
	reserveArrays[i][j] = (double*)exa_calloc(length * LENGTH_LNL_ARRAY, sizeof(double)); // TODO not aligned? 


      partitionScaler[i] = (nat*)exa_calloc(2 * tr->mxtips , sizeof(nat)); 
    }  
  
  orientation = (int*) exa_calloc(tr->mxtips, sizeof(int)); 
  wasSwitched = (bool*)exa_calloc( 2 * tr->mxtips , sizeof(bool));
}




LnlRestorer::~LnlRestorer()  
{
  tree *tr = chain->traln->getTr(); 
  int numPart = chain->traln->getNumberOfPartitions(); 
  for(int i = 0; i <  numPart; ++i)
    {
      for(int j = 0; j < tr->mxtips; ++j)
	{
	  exa_free(reserveArrays[i][j]);
	}
      exa_free(reserveArrays[i]); 
      
      exa_free(partitionScaler[i]); 
    }
  exa_free(reserveArrays); 
  exa_free(partitionScaler); 
  exa_free(wasSwitched); 
}



/**
   @brief loads the saved orientation of the x-vectors
 */
void LnlRestorer::loadOrientation()
{
  tree *tr = chain->traln->getTr(); 
  int ctr = 0; 
  for(int i = tr->mxtips+1; i < 2 * tr->mxtips-1; ++i)
    {
      int val = orientation[ctr]; 
      branch b = constructBranch(tr->nodep[i]->number, val ); 
      branchExists(tr, b); 
      
      nodeptr q = findNodeFromBranch(tr, b);
      assert(q->back->number == val); 
      q->x = 1; q->next->x = 0; q->next->next->x = 0;
      
      ctr++; 
    } 
}


void LnlRestorer::storeOrientation()
{
  tree *tr = chain->traln->getTr(); 
  int ctr = 0; 
  for(int i = tr->mxtips+1; i < 2 * tr->mxtips-1; ++i)
    {	
      int *val = orientation + ctr ; 
      nodeptr p = tr->nodep[i]; 
      if(p->x)
	*val = p->back->number; 
      else if(p->next->x)
	*val = p->next->back->number; 
      else if(p->next->next->x)
	*val = p->next->next->back->number; 
      else 
	assert(0); 
      ctr++; 
    }
}



void LnlRestorer::swapArray(int nodeNumber, int model)
{
  tree *tr = chain->traln->getTr();   
  assert(NOT isTip(nodeNumber, tr->mxtips)); 
  int numPart = chain->traln->getNumberOfPartitions(); 
  
  if(model == ALL_MODELS)
    {
#ifdef DEBUG_ARRAY_SWAP
      if(isOutputProcess())
      cout << "swapped array for node " << nodeNumber <<   " and all  models "; 
#endif

      for(int i = 0; i < numPart; ++i)
	{
	  pInfo *partition = chain->traln->getPartition( i);
	  int posInArray = nodeNumber -( tr->mxtips + 1); 	  
	  double *&a =  reserveArrays[i][posInArray], 
	    *&b  = partition->xVector[posInArray] ; 
#ifdef DEBUG_ARRAY_SWAP
	  if(isOutputProcess())
	  cout << a << "," << b << "\t"; 
#endif
	  if(NOT b )
	    return; 
	  swap(a,b); 	  
	}
#ifdef DEBUG_ARRAY_SWAP
      cout << endl; 
#endif
    }
  else 
    {
      pInfo *partition = chain->traln->getPartition( model); 
      int posInArray = nodeNumber- (tr->mxtips+1); 
      double*& a =  reserveArrays[model][posInArray], 
	*&b  = partition->xVector[posInArray]; 

#ifdef  DEBUG_ARRAY_SWAP
      if(isOutputProcess())
      cout << "swapped array for node " << nodeNumber <<  " and model " << model << ": " << a   << "," << b  << endl; 
#endif
      if(NOT b )
	return; 
      swap(a,b );     
    }
}





void LnlRestorer::restore()
{
#ifdef DEBUG_ARRAY_SWAP 
  if(isOutputProcess())
  cout << "RESTORE for model" << modelEvaluated << endl; 
#endif
  tree *tr = chain->traln->getTr(); 
  int numPart =  chain->traln->getNumberOfPartitions(); 
  // switch arrays 
  for(int i = 0; i < 2 * tr->mxtips; ++i)
    {
      if(wasSwitched[i])
	swapArray(i , modelEvaluated); 
    }

  loadOrientation();

  for(int i = 0; i < numPart; ++i)
    {
      pInfo *partition = chain->traln->getPartition( i); 
      memcpy(partition->globalScaler, partitionScaler[i], sizeof(nat) * 2 * chain->traln->getTr()->mxtips);     
    }
  

#ifdef DEBUG_LNL_VERIFY
  nodeptr p = findNodeFromBranch(tr, findRoot(tr)); 
  evaluatePartialNoBackup(chain, p);   
  double diff = fabs(prevLnl - chain->traln->getTr()->likelihood); 
  if( diff > 1e-6 )
    {
      cout << "problem restoring previous tr/aln state DIFF: " <<  diff  << "\t" << "(" << prevLnl << "," << chain->traln->getTr()->likelihood << ")"  << endl; 
      assert(0); 
    }
#endif

  tr->likelihood = prevLnl; 

}





void LnlRestorer::traverseAndSwitchIfNecessary(nodeptr virtualRoot, int model, bool fullTraversal)
{  
  this->modelEvaluated = model; 
  tree *tr = chain->traln->getTr(); 
  // int numPart = getNumberOfPartitions(tr); 

  if(isTip(virtualRoot->number, tr->mxtips))
    return; 

  bool incorrect = NOT virtualRoot->x; 
  if( ( incorrect
	|| fullTraversal )      
      && NOT wasSwitched[virtualRoot->number])
    {
#ifdef DEBUG_ARRAY_SWAP
      if(isOutputProcess())
      cout << "incorr, unseen " << virtualRoot->number << endl; 
#endif
      wasSwitched[virtualRoot->number] = true; 
      
      swapArray(virtualRoot->number, model); 
    }
  else if (incorrect)
    {
#ifdef DEBUG_ARRAY_SWAP
      if(isOutputProcess())
      cout << "incorr, seen " <<  virtualRoot->number  << endl; 
#endif
    }
#ifdef DEBUG_ARRAY_SWAP
  else
    if(isOutputProcess())
    cout << "corr "<<  virtualRoot->number << endl; 
#endif

  if(incorrect || fullTraversal)
    {
      traverseAndSwitchIfNecessary(virtualRoot->next->back, model, fullTraversal); 
      traverseAndSwitchIfNecessary(virtualRoot->next->next->back, model, fullTraversal); 
    }
}




/**@brief  resets the restorer, s.t. it is consistent with the current tree (and can restore it later) */ 
void LnlRestorer::resetRestorer()
{
#ifdef DEBUG_ARRAY_SWAP
  if(isOutputProcess())
  cout << "RESETTING RESTORER" << endl; 
#endif
  tree *tr = chain->traln->getTr();
  int numPart = chain->traln->getNumberOfPartitions(); 
  memset(wasSwitched, 0, sizeof(bool) * ( 2 * tr->mxtips) ); 
  storeOrientation(); 
  // cout << "scaler is :" << endl; 
  for(int i = 0; i < numPart; ++i)
    {
      pInfo *partition = chain->traln->getPartition( i); 
      memcpy(partitionScaler[i], partition->globalScaler, sizeof(nat) * 2 * chain->traln->getTr()->mxtips);     
      // for(int j = 0 ; j < 2 * tr->mxtips; ++j )
	// cout << partitionScaler[i][j] << "," ; 
      // cout << endl; 
    }
  modelEvaluated = -1; 
  prevLnl = tr->likelihood;   
}
