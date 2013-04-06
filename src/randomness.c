


#include <math.h>
#include <inttypes.h>

#include "axml.h"
#include "main-common.h"
#include "bayes.h"
#include "globals.h"
#include "randomness.h"
#include "branch.h"
#include "adapters.h"


static void inc_global()
{
  if(gAInfo.rGlobalCtr.v[0] == 2147483645) /* roughly what a int32_t can hold */
    {
      gAInfo.rGlobalCtr.v[1]++; 
      gAInfo.rGlobalCtr.v[0] = 0;       
    }
  else 
    gAInfo.rGlobalCtr.v[0]++; 
  
  assert(gAInfo.rGlobalCtr.v[1] < 2147483645); 
}




void initLocalRng(state *theChain)
{
  /* init rng */
  theChain->rCtr.v[0] = 0; 
  theChain->rCtr.v[1] = 0; 
  randCtr_t r = drawGlobalRandInt();
  theChain->rKey.v[0] = r.v[0]; 
  theChain->rKey.v[1] = r.v[1]; 
  if(processID == 0)
    printf("initialized chain %d with seed %d,%d\n", theChain->id, theChain->rKey.v[0], theChain->rKey.v[1]); 
}


randCtr_t drawGlobalRandInt()
{
  randCtr_t result = exa_rand(gAInfo.rGlobalKey, gAInfo.rGlobalCtr); 
  inc_global();
  return result; 
}

int drawGlobalRandIntBound(int upperBound)
{
  randCtr_t r = drawGlobalRandInt();
  return r.v[0] % upperBound; 
}



double drawGlobalDouble01()
{
  randCtr_t result  = exa_rand(gAInfo.rGlobalKey, gAInfo.rGlobalCtr);   
  return u01_closed_open_32_53(result.v[1]) ; 
}



/* chain specific  */



/* uniform r in  [0,upperBound-1] */
int drawRandInt(state *chain, int upperBound )
{
  chain->rCtr.v[0] =  chain->currentGeneration; 
  randCtr_t r = exa_rand(chain->rKey, chain->rCtr); 
  chain->rCtr.v[1]++;   
  return r.v[0] % upperBound; 
}


/* uniform r in [0,1) */
double drawRandDouble01(state *chain)
{
  chain->rCtr.v[0] = chain->currentGeneration; 
  randCtr_t r = exa_rand(chain->rKey, chain->rCtr); 
  chain->rCtr.v[1]++; 
  return u01_closed_open_32_53(r.v[1]); 
}



double drawRandExp(state *chain, double lambda)
{  
  double r = drawRandDouble01(chain);   
  return -log(r )/ lambda; 
}


double drawRandBiUnif(state *chain, double x)
{
  double r = drawRandDouble01(chain) *  (2*x-x/2) + x / (3/2) ; 
  return r; 
}


//Given x this function randomly draws from [x/2,2*x] 
/* double drawRandBiUnif(double x) */
/* { */
/*   double r; */
/*   //r=(double)rand()/(double)RAND_MAX;   */
/*   //r=x/2+r*(3/2)*x;//=x/2+r*(2*x-x/2); */
  
/*   r=drawRandDouble(2*x-x/2)+x/2; */
/*   //r=drawRandDouble((3/2)*x-x/(3/2))+x/(3/2); */
  
/*   return r; */
/* } */


/**
   @brief draw r according to distribution given by weights. 

   NOTE sum of weights is not required to be 1.0
*/
int drawSampleProportionally(state *chain,  double *weights, int numWeight )
{
  double r = drawRandDouble01(chain);
 
  double sum=0.0;
  double lower_bound = 0.0;
  size_t i = 0;
  
  assert( numWeight > 0 );
  
  for( i = 0; i < numWeight ; ++i ) 
    {
      sum+=weights[i]; 
    }
  assert(sum>0);
  r=r*sum;
    
  for( i = 0; i < numWeight ; ++i ) 
    {
      double upper_bound = lower_bound + weights[i];
    
      if( r >= lower_bound && r < upper_bound ) 
	return i ;
    
      lower_bound = upper_bound; 
    }
  
  return i-1;
}

//get random permutation of [0,n-1]
void drawPermutation(state *chain, int* perm, int n)
{
  int i;
  int randomNumber;
  perm[0] = 0;
  
  for(i=1 ; i<n ; i++){
  
    randomNumber = drawRandInt(chain, i+1);
    // randomNumber=rand() % (i+1);
    // randomNumber=rand();

    if(randomNumber==i){
      perm[i]=i;
    }else{
      perm[i]=perm[randomNumber];
      perm[randomNumber]=i;
    }
  }
  
  /*for(i=0 ; i<n ; i++){
    printf("%d ",perm[i]);

    
    }
    printf("\n");
  */
} 



/**
   @brief draws a subtree uniformly. 

   We have to account for tips again. 
   
   The thisNode contains the root of the subtree.
*/ 
branch drawSubtreeUniform(state *chain)
{
  tree *tr = chain->tr;   
  while(TRUE)
    {
      branch b = drawBranchUniform(chain); 
      if(isTipBranch(b,tr->mxtips))
	{
	  if(isTip(b.thisNode, tr->mxtips))
	    b = invertBranch(b); 
	  if(drawRandDouble01(chain) < 0.5)
	    return b; 
	}
      else 
	{
	  return drawRandDouble01(chain) < 0.5 ? b  : invertBranch(b); 	  
	}
    }
}


/**
   @brief draws a branch with uniform probability.
   
   We have to treat inner and outer branches separatedly.
 */
branch drawBranchUniform(state *chain)
{
  tree *tr = chain->tr; 

  boolean accept = FALSE; 
  int randId = 0; 
  while(NOT accept)
    {
      randId = drawRandInt(chain, 2 * tr->mxtips - 2 ) + 1;
      assert(randId > 0); 
      double r = drawRandDouble01(chain); 
      if(isTip(randId, tr->mxtips) )
	accept = r < 0.25 ; 
      else 
	accept = r <= 0.75; 	
    }

  branch result; 
  result.thisNode = randId; 
  nodeptr p = tr->nodep[randId]; 
  if(isTip(randId, tr->mxtips))
    result.thatNode = p->back->number; 
  else 
    {
      int r = drawRandInt(chain,2); 
      switch(r)
	{
	case 0 : 
	  result.thatNode = p->back->number; 
	  break; 
	case 1 : 
	  result.thatNode = p->next->back->number; 
	  break; 
	case 2: 
	  result.thatNode = p->next->next->back->number; 
	  break; 
	default: assert(0); 
	}
    }
  
  return result; 
}


/**
   @brief gets a multiplier for updating a parameter or branch length 
 */
double drawMultiplier(state *chain, double multiplier)
{
  double tmp =  exp(multiplier * (drawRandDouble01(chain)  - 0.5)); 
  assert(tmp > 0.); 
  return tmp ; 
  
}


double drawFromSlidingWindow(state *chain, double param, double window)
{
  double upper = param + (window / 2 ) ,
    lower = param - (window / 2 ); 
  
  double r = drawRandDouble01(chain); 
  return lower + r * (upper - lower)  ; /* TODO correct?  */
}
