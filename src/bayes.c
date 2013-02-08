

#include "common.h"

/* #include "cycle.h" */
#include "axml.h"
#include "proposals.h"
#include "randomness.h"
#include "globals.h"


extern int Thorough;
/* extern char run_id[128]; */
extern int processID;
extern int seed; 



#define _USE_NCL_PARSER

#ifdef _USE_NCL_PARSER
#include "nclConfigReader.h"
void addInitParameters(state *curstate, initParamStruct *initParams)
{
  curstate->proposalWeights[SPR] = initParams->initSPRWeight; 
  curstate->proposalWeights[UPDATE_GAMMA] = initParams->initGammaWeight;
  curstate->proposalWeights[UPDATE_MODEL] = initParams->initModelWeight; 
  curstate->proposalWeights[UPDATE_SINGLE_BL] = initParams->initSingleBranchWeight; 
  curstate->proposalWeights[UPDATE_SINGLE_BL_EXP] = initParams->initSingleBranchExpWeight; 
  curstate->numGen = initParams->numGen; 
  curstate->penaltyFactor = initParams->initPenaltyFactor; 
}
#else 
int parseConfig(state *theState);
#endif


//reads proposalWeights p and sets t for logistic function such that f(t)=p
void findLogisticT(state *curstate){
  static double leftShift=2.0;
   for(int i = 0; i < NUM_PROPOSALS;++i){
     if(curstate->proposalWeights[i]>0){
    curstate->proposalLogisticT[i]=-log((1.0-curstate->proposalWeights[i])/curstate->proposalWeights[i])+leftShift;  
    //printf("p(i): %f e^..: %f\n",curstate->proposalWeights[i], (1.0/(1.0+exp(leftShift-curstate->proposalLogisticT[i])) ));
     }else{
       curstate->proposalLogisticT[i]=-1.0;
     }
   }
}

//get values for logistic function
void findLogisticP(state *curstate){
  static double leftShift=2.0;
  for(int i = 0; i < NUM_PROPOSALS;++i){
    if(curstate->proposalLogisticT[i]>=0){
    curstate->proposalWeights[i]=(1.0/(1.0+exp(leftShift-curstate->proposalLogisticT[i])) );  
  //  printf("p(i): %f e^..: %f t: %f\n",curstate->proposalWeights[i], (1.0/(1.0+exp(leftShift-curstate->proposalLogisticT[i])) ), curstate->proposalLogisticT[i]);
   }
  }
}



/* TODO commented this out, since there are some problems with it,
   when we build with the PLL */
/* #define WITH_PERFORMANCE_MEASUREMENTS */


//NOTE: should only be called at the very beginning. Afterwards a probability sum of 1.0 is not required.

void normalizeProposalWeights(state *curstate)
{
  double sum = 0 ; 
  for(int i = 0; i < NUM_PROPOSALS;++i)
    sum += curstate->proposalWeights[i]; 
  
  for(int i = 0; i < NUM_PROPOSALS;++i)
    curstate->proposalWeights[i] /= sum; 
  
  findLogisticT(curstate);
}





state *state_init(tree *tr, analdef * adef, double bl_w, double rt_w, double gm_w, double bl_p)
{
  state *curstate  =(state *)calloc(1,sizeof(state));
  nodeptr *list = (nodeptr *)malloc(sizeof(nodeptr) * 2 * tr->mxtips);
  curstate->list = list;

  curstate->tr = tr;
  curstate->brLenRemem.bl_sliding_window_w = bl_w;
  curstate->brLenRemem.bl_prior = 1.0;
  curstate->brLenRemem.bl_prior_exp_lambda = bl_p;
  //this can be extended to more than one partition, but one for now
  curstate->modelRemem.model = 0;
  curstate->modelRemem.adef = adef;
  curstate->modelRemem.rt_sliding_window_w = rt_w;
  curstate->modelRemem.nstates = tr->partitionData[curstate->modelRemem.model].states; /* 4 for DNA */
  curstate->modelRemem.numSubsRates = (curstate->modelRemem.nstates * curstate->modelRemem.nstates - curstate->modelRemem.nstates) / 2; /* 6 for DNA */
  curstate->modelRemem.curSubsRates = (double *) malloc(curstate->modelRemem.numSubsRates * sizeof(double));
  curstate->gammaRemem.gm_sliding_window_w = gm_w;
  assert(curstate != NULL);
  
  curstate->brLenRemem.single_bl_branch = -1;

  return curstate;
}





static void printSubsRates(tree *tr,int model, int numSubsRates)
{
  assert(tr->partitionData[model].dataType = DNA_DATA);
  int i;
  printBothOpen("Subs rates: ");
  for(i=0; i<numSubsRates; i++)
    printBothOpen("%d => %.3f, ", i, tr->partitionData[model].substRates[i]);
  printBothOpen("\n\n");
}

static void recordSubsRates(tree *tr, int model, int numSubsRates, double *prevSubsRates)
{
  assert(tr->partitionData[model].dataType = DNA_DATA);
  int i;
  for(i=0; i<numSubsRates; i++)
    prevSubsRates[i] = tr->partitionData[model].substRates[i];
}


static void reset_branch_length(nodeptr p, int numBranches)
{
  int i;
  for(i = 0; i < numBranches; i++)
    {
      assert(p->z_tmp[i] == p->back->z_tmp[i]);
      p->z[i] = p->back->z[i] = p->z_tmp[i];   /* restore saved value */
    }
}



//setting this out to allow for other types of setting
static void set_branch_length_sliding_window(nodeptr p, int numBranches,state * s, boolean record_tmp_bl)
{
  int i;
  double new_value;
  double r,mx,mn;
  for(i = 0; i < numBranches; i++)
    {
      double real_z;
    
      if(record_tmp_bl)
	{
	  assert(p->z[i] == p->back->z[i]); 
	  p->z_tmp[i] = p->back->z_tmp[i] = p->z[i];   /* keep current value */
	}
      r = drawRandDouble();

      real_z = -log(p->z[i]) * s->tr->fracchange;
    
      //     printf( "z: %f %f\n", p->z[i], real_z );
    
      mn = real_z-(s->brLenRemem.bl_sliding_window_w/2);
      mx = real_z+(s->brLenRemem.bl_sliding_window_w/2);
      new_value = exp(-(fabs(mn + r * (mx-mn)/s->tr->fracchange )));
    
    
      /* Ensure always you stay within this range */
      if(new_value > zmax) new_value = zmax;
      if(new_value < zmin) new_value = zmin;
      assert(new_value <= zmax && new_value >= zmin);
      //     printf( "z: %f %f %f\n", p->z[i], new_value, real_z );
      p->z[i] = p->back->z[i] = new_value;
      //assuming this will be visiting each node, and multiple threads won't be accessing this
      //s->bl_prior += log(exp_pdf(s->bl_prior_exp_lambda,new_value));
      //s->bl_prior += 1;
    }
}

static void set_branch_length_exp(nodeptr p, int numBranches,state * s, boolean record_tmp_bl)
{
 // static double lambda=10; //TODO should be defined elsewhere. Also: must find lambda to yield good proposals
  
  int i;
  double new_value;
  double r;
  double real_z;
  for(i = 0; i < numBranches; i++)
    {
     // double real_z;
    
      if(record_tmp_bl)
	{
	  assert(p->z[i] == p->back->z[i]); 
	  p->z_tmp[i] = p->back->z_tmp[i] = p->z[i];   /* keep current value */
	}
      //r = drawRandExp(lambda);
       real_z = -log(p->z[i]) * s->tr->fracchange;
	r = drawRandExp(1.0/real_z);

      new_value = exp(-(fabs(r /s->tr->fracchange )));
   
    
      /* Ensure always you stay within this range */
      if(new_value > zmax) new_value = zmax;
      if(new_value < zmin) new_value = zmin;
      assert(new_value <= zmax && new_value >= zmin);
      p->z[i] = p->back->z[i] = new_value;
     }
}


static node *select_branch_by_id_dfs_rec( node *p, int *cur_count, int target, state *s ) {
  if( (*cur_count) == target ) {
    return p;
  }
  
  if( !isTip( p->number, s->tr->mxtips )) {
    // node *q = p->back;
    node *ret = NULL;
    
    ++(*cur_count);
    ret = select_branch_by_id_dfs_rec( p->next->back, cur_count, target, s );
    if( ret != NULL ) {
      return ret;
    }
    
    ++(*cur_count);
    ret = select_branch_by_id_dfs_rec( p->next->next->back, cur_count, target, s );
    
    return ret;
  } else {
    return NULL;
  }
}


static node *select_branch_by_id_dfs( node *p, int target, state *s ) {
  const int num_branches = (2 * s->tr->mxtips) - 3;
  
  //   if( target == num_branches ) {
  //     return NULL;
  //   } 
  
  assert( target < num_branches );
  
  int cur_count = 0;
  node *ret = NULL;
  
  
  ret = select_branch_by_id_dfs_rec( p, &cur_count, target, s );
  
  if( ret != NULL ) {
    return ret;
  } else {
    // not in subtree below p. search on in the subtrees below p->back
  
    node *q = p->back;
    
    if( isTip( q->number, s->tr->mxtips ) ) {
      assert( 0 ); // error: target must be invalid 'branch id'
      return NULL; 
    }
    
    ++cur_count;
    ret = select_branch_by_id_dfs_rec( q->next->back, &cur_count, target, s );
  
    if( ret != NULL ) {
      return ret;
    }
    
    
    ++cur_count;
    ret = select_branch_by_id_dfs_rec( q->next->next->back, &cur_count, target, s );
  
    return ret;
  
  }
  
  assert(0);
  
  
}


static void traverse_branches_set_fixed(nodeptr p, int *count, state * s, double z )
{
  nodeptr q;
  int i;
  //printf("current BL at %db%d: %f\n", p->number, p->back->number, p->z[0]);
  
  for( i = 0; i < s->tr->numBranches; i++)
    {
   
      p->z[i] = p->back->z[i] = z;
    }
  
  *count += 1;


  if (! isTip(p->number, s->tr->mxtips)) 
    {                                  /*  Adjust descendants */
      q = p->next;
      while (q != p) 
	{
	  traverse_branches_set_fixed(q->back, count, s, z);
	  q = q->next;
	}   
      newviewGeneric(s->tr, p, FALSE);     // not sure if we need this
    }
}


/*
 * should be sliding window proposal
 */

static void random_branch_length_proposal_apply(state * instate)
{
   
  //for each branch get the current branch length
  //pull a uniform like
  //x = current, w =window
  //uniform(x-w/2,x+w/2)
  
  
  const int num_branches = (instate->tr->mxtips * 2) - 3;
  int target_branch = drawRandInt(num_branches); 
  node *p = select_branch_by_id_dfs( instate->tr->start, target_branch, instate );
  
  
  //   printf( "apply bl: %p %f\n", p, p->z[0] );
  set_branch_length_sliding_window(p, instate->tr->numBranches, instate, TRUE);

  instate->brLenRemem.single_bl_branch = target_branch;
  evaluateGeneric(instate->tr, instate->tr->start, TRUE); /* update the tr->likelihood *///TODO see below
  //   return TRUE;
}

static void exp_branch_length_proposal_apply(state * instate)
{
  const int num_branches = (instate->tr->mxtips * 2) - 3;
  int target_branch = drawRandInt(num_branches); 
  node *p = select_branch_by_id_dfs( instate->tr->start, target_branch, instate );
  
  set_branch_length_exp(p, instate->tr->numBranches, instate, TRUE);

  instate->brLenRemem.single_bl_branch = target_branch;
  evaluateGeneric(instate->tr, instate->tr->start, TRUE); /* update the tr->likelihood *///TODO see below
}

static void random_branch_length_proposal_reset(state * instate)
{
  node *p;
  assert( instate->brLenRemem.single_bl_branch != -1 );
  
  // ok, maybe it would be smarter to store the node ptr for rollback rather than re-search it...
  p = select_branch_by_id_dfs( instate->tr->start, instate->brLenRemem.single_bl_branch, instate );
  
  reset_branch_length(p, instate->tr->numBranches);
  //   printf( "reset bl: %p %f\n", p, p->z[0] );
  //update_all_branches(instate, TRUE);
  evaluateGeneric(instate->tr, instate->tr->start, TRUE); /* update the tr->likelihood *///TODO should this not be evaluateGeneric(...,FALSE)?
  instate->brLenRemem.single_bl_branch = -1;
}



/*
 * should be sliding window proposal
 */

static void edit_subs_rates(tree *tr, int model, int subRatePos, double subRateValue)
{
  assert(tr->partitionData[model].dataType = DNA_DATA);
  assert(subRateValue <= RATE_MAX && subRateValue >= RATE_MIN);
  int states = tr->partitionData[model].states; 
  int numSubsRates = (states * states - states) / 2;
  assert(subRatePos >= 0 && subRatePos < numSubsRates);
  tr->partitionData[model].substRates[subRatePos] = subRateValue;
}

static void simple_model_proposal_apply(state *instate)
{
  //TODO: add safety to max and min values
  //record the old ones
  recordSubsRates(instate->tr, instate->modelRemem.model, instate->modelRemem.numSubsRates, instate->modelRemem.curSubsRates);
  //choose a random set of model params,
  //probably with dirichlet proposal
  //with uniform probabilities, no need to have other
  int state;
  double new_value,curv;
  double r,mx,mn;
  //using the branch length sliding window for a test
  for(state = 0;state<instate->modelRemem.numSubsRates ; state ++)
    {
      curv = instate->tr->partitionData[instate->modelRemem.model].substRates[state];
      r =  drawRandDouble();
      mn = curv-(instate->modelRemem.rt_sliding_window_w/2);
      mx = curv+(instate->modelRemem.rt_sliding_window_w/2);
      new_value = fabs(mn + r * (mx-mn));
      /* Ensure always you stay within this range */
      if(new_value > RATE_MAX) new_value = RATE_MAX;
      if(new_value < RATE_MIN) new_value = RATE_MIN;
      //printf("%i %f %f\n", state, curv, new_value);
      edit_subs_rates(instate->tr,instate->modelRemem.model, state, new_value);
    }
  //recalculate eigens

  initReversibleGTR(instate->tr, instate->modelRemem.model); /* 1. recomputes Eigenvectors, Eigenvalues etc. for Q decomp. */

  /* TODO: need to broadcast rates here for parallel version ! */

  evaluateGeneric(instate->tr, instate->tr->start, TRUE); /* 2. re-traverse the full tree to update all vectors */
  //TODO: without this, the run will fail after a successful model, but failing SPR
  //TODOFER: what did we have in mind regarding the comment above?
  
  evaluateGeneric(instate->tr, instate->tr->start, FALSE);
  //for prior, just use dirichlet
  // independent gamma distribution for each parameter
  //the pdf for this is
  // for gamma the prior is gamma

  //for statefreqs should all be uniform

  //only calculate the new ones
}

static void restore_subs_rates(tree *tr, analdef *adef, int model, int numSubsRates, double *prevSubsRates)
{
  assert(tr->partitionData[model].dataType = DNA_DATA);
  int i;
  for(i=0; i<numSubsRates; i++)
    tr->partitionData[model].substRates[i] = prevSubsRates[i];

  initReversibleGTR(tr, model);

  /* TODO need to broadcast rates here for parallel version */

  evaluateGeneric(tr, tr->start, TRUE);
}

static void simple_model_proposal_reset(state * instate)
{
  restore_subs_rates(instate->tr, instate->modelRemem.adef, instate->modelRemem.model, instate->modelRemem.numSubsRates, instate->modelRemem.curSubsRates);
  //evaluateGeneric(instate->tr, instate->tr->start, FALSE);
}

static nodeptr select_random_subtree(tree *tr)
{
  nodeptr 
    p;

  do
    {
      int 
        exitDirection = drawRandInt(3); 
     
      int r = drawRandInt(tr->mxtips - 2) ; 

      p = tr->nodep[ r + 1 + tr->mxtips];
      
      switch(exitDirection)
        {
        case 0:
          break;
        case 1:
          p = p->next;
          break;
        case 2:
          p = p->next->next;
          break;
        default:
          assert(0);
        }
    }
  while(isTip(p->next->back->number, tr->mxtips) && isTip(p->next->next->back->number, tr->mxtips));

  assert(!isTip(p->number, tr->mxtips));

  return p;
}

static void record_branch_info(nodeptr p, double *bl, int numBranches)
{
  int i;
  for(i = 0; i < numBranches; i++)
    bl[i] = p->z[i];
}
static int spr_depth = 0;

static node *random_spr_traverse( tree *tr, node *n ) {

  double randprop = drawRandDouble();
  
  if( isTip(n->number, tr->mxtips ) || randprop < 0.5 ) {
    return n;
  } else if( randprop < 0.75 ) {
    spr_depth++;
    return random_spr_traverse( tr, n->next->back );
  } else {
    spr_depth++;
    return random_spr_traverse( tr, n->next->next->back );
  }
}
static node *random_spr( tree *tr, node *n ) {
  if( isTip(n->number, tr->mxtips ) ) {
    return n;
  }
  spr_depth = 1;
  double randprop = drawRandDouble();
  if( randprop < 0.5 ) {
    return random_spr_traverse( tr, n->next->back );
  } else {
    return random_spr_traverse( tr, n->next->next->back );
  }
}

static void random_spr_apply(state *instate)
{
  tree * tr = instate->tr;
  
  nodeptr    
    p = select_random_subtree(tr);
  
  /* evaluateGeneric(tr, tr->start, TRUE);
     printf("%f \n", tr->likelihood);*/

#if 0
  parsimonySPR(p, tr);
#endif

  

  /*evaluateGeneric(tr, tr->start, TRUE);
    printf("%f \n", tr->likelihood);*/

  instate->sprMoveRemem.p = p;
  instate->sprMoveRemem.nb  = p->next->back;
  instate->sprMoveRemem.nnb = p->next->next->back;
  
  record_branch_info(instate->sprMoveRemem.nb, instate->sprMoveRemem.nbz, instate->tr->numBranches);
  record_branch_info(instate->sprMoveRemem.nnb, instate->sprMoveRemem.nnbz, instate->tr->numBranches);

  /* removeNodeBIG(tr, p,  tr->numBranches); */
  /* remove node p */
  double   zqr[NUM_BRANCHES];
  int i;
  for(i = 0; i < tr->numBranches; i++)
    {
      zqr[i] = instate->sprMoveRemem.nb->z[i] * instate->sprMoveRemem.nnb->z[i];
      if(zqr[i] > zmax) zqr[i] = zmax;
      if(zqr[i] < zmin) zqr[i] = zmin;
    }
  hookup(instate->sprMoveRemem.nb, instate->sprMoveRemem.nnb, zqr, tr->numBranches); 
  p->next->next->back = p->next->back = (node *) NULL;
  /* done remove node p (omitted BL opt) */

  
  double randprop = drawRandDouble();
  spr_depth = 0;
  assert( !(isTip(instate->sprMoveRemem.nb->number, instate->tr->mxtips ) && isTip(instate->sprMoveRemem.nnb->number, instate->tr->mxtips )) );
  if( isTip(instate->sprMoveRemem.nb->number, instate->tr->mxtips ) ) {
    randprop = 1.0;
  } else if( isTip(instate->sprMoveRemem.nnb->number, instate->tr->mxtips ) ) {
    randprop = 0.0;
  }
  
  if( randprop <= 0.5 ) {
    tr->insertNode = random_spr( tr, instate->sprMoveRemem.nb );
  } else {
    tr->insertNode = random_spr( tr, instate->sprMoveRemem.nnb );
  }

  

  instate->sprMoveRemem.q = tr->insertNode;
  instate->sprMoveRemem.r = instate->sprMoveRemem.q->back;
  record_branch_info(instate->sprMoveRemem.q, instate->brLenRemem.qz, instate->tr->numBranches);

  
  Thorough = 0;
  // assert(tr->thoroughInsertion == 0);
  /* insertBIG wont change the BL if we are not in thorough mode */
  
  insertBIG(instate->tr, instate->sprMoveRemem.p, instate->sprMoveRemem.q, instate->tr->numBranches);
  evaluateGeneric(instate->tr, instate->sprMoveRemem.p->next->next, FALSE); 
  /*testInsertBIG(tr, p, tr->insertNode);*/

  //printf("%f \n", tr->likelihood);
}

static void random_spr_reset(state * instate)
{
  /* prune the insertion */
  hookup(instate->sprMoveRemem.q, instate->sprMoveRemem.r, instate->brLenRemem.qz, instate->tr->numBranches);
  instate->sprMoveRemem.p->next->next->back = instate->sprMoveRemem.p->next->back = (nodeptr) NULL;
  /* insert the pruned tree in its original node */
  hookup(instate->sprMoveRemem.p->next,       instate->sprMoveRemem.nb, instate->sprMoveRemem.nbz, instate->tr->numBranches);
  hookup(instate->sprMoveRemem.p->next->next, instate->sprMoveRemem.nnb, instate->sprMoveRemem.nnbz, instate->tr->numBranches);
  newviewGeneric(instate->tr, instate->sprMoveRemem.p, FALSE); 
}


//simple sliding window
static void simple_gamma_proposal_apply(state * instate)
{
  //TODO: add safety to max and min values
  double newalpha, curv, r,mx,mn;
  curv = instate->tr->partitionData[instate->modelRemem.model].alpha;
  instate->gammaRemem.curAlpha = curv;
  r = drawRandDouble();
  mn = curv-(instate->gammaRemem.gm_sliding_window_w/2);
  mx = curv+(instate->gammaRemem.gm_sliding_window_w/2);
  newalpha = fabs(mn + r * (mx-mn));
  /* Ensure always you stay within this range */
  if(newalpha > ALPHA_MAX) newalpha = ALPHA_MAX;
  if(newalpha < ALPHA_MIN) newalpha = ALPHA_MIN;
  instate->tr->partitionData[instate->modelRemem.model].alpha = newalpha;

  makeGammaCats(instate->tr->partitionData[instate->modelRemem.model].alpha, instate->tr->partitionData[instate->modelRemem.model].gammaRates, 4, instate->tr->useMedian);

  
  evaluateGeneric(instate->tr, instate->tr->start, TRUE);
}

static void simple_gamma_proposal_reset(state * instate)
{
  instate->tr->partitionData[instate->modelRemem.model].alpha = instate->gammaRemem.curAlpha;

  makeGammaCats(instate->tr->partitionData[instate->modelRemem.model].alpha, instate->tr->partitionData[instate->modelRemem.model].gammaRates, 4, instate->tr->useMedian);

  evaluateGeneric(instate->tr, instate->tr->start, TRUE);
}


static proposal_functions get_proposal_functions( proposal_type ptype ) {
  
  //TODO do we really need to define NUM_PROP_FUNCS in addition to NUM_PROPOSALS
#define NUM_PROP_FUNCS (5) 
  const static proposal_functions prop_funcs[NUM_PROP_FUNCS] = { 
    { UPDATE_MODEL, simple_model_proposal_apply, simple_model_proposal_reset },
    { UPDATE_SINGLE_BL, random_branch_length_proposal_apply, random_branch_length_proposal_reset },
    { UPDATE_SINGLE_BL_EXP, exp_branch_length_proposal_apply, random_branch_length_proposal_reset },
    { UPDATE_GAMMA, simple_gamma_proposal_apply, simple_gamma_proposal_reset },
    { SPR, random_spr_apply, random_spr_reset }
  };
  int i;
  // REMARK: don't worry about the linear search until NUM_PROP_FUNCS exceeds 20...
  for( i = 0; i < NUM_PROP_FUNCS; ++i ) {
    if( prop_funcs[i].ptype == ptype ) {
      return prop_funcs[i];
    }
  }
  
  assert( 0 && "ptype not found" );
  
#undef NUM_PROP_FUNCS
}

static void dispatch_proposal_apply( proposal_type ptype, state *instate ) {
  get_proposal_functions(ptype).apply_func( instate );
}

static void dispatch_proposal_reset( proposal_type ptype, state *instate ) {
  get_proposal_functions(ptype).reset_func( instate );
}






/* so here the idea would be to randomly choose among proposals? we can use typedef enum to label each, and return that */ 
static proposal_type select_proposal_type(state * instate)
{
  instate->newprior = instate->brLenRemem.bl_prior;  
  return drawSampleProportionally(instate->proposalWeights, NUM_PROPOSALS) ; 
}


static node *find_tip( node *n, tree *tr ) {
  if( isTip(n->number, tr->mxtips) ) {
    return n;
  } else {
    return find_tip( n->back, tr );
  }
  
}

#if 0
static char *Tree2StringRecomREC(char *treestr, tree *tr, nodeptr q, boolean printBranchLengths)
{
  char  *nameptr;            
  double z;
  nodeptr p = q;

  if(isTip(p->number, tr->mxtips)) 
    {               
      nameptr = tr->nameList[p->number];     
      sprintf(treestr, "%s", nameptr);
      while (*treestr) treestr++;
    }
  else 
    {                      
      while(!p->x)
	p = p->next;
      *treestr++ = '(';
      treestr = Tree2StringRecomREC(treestr, tr, q->next->back, printBranchLengths);
      *treestr++ = ',';
      treestr = Tree2StringRecomREC(treestr, tr, q->next->next->back, printBranchLengths);
      if(q == tr->start->back) 
	{
	  *treestr++ = ',';
	  treestr = Tree2StringRecomREC(treestr, tr, q->back, printBranchLengths);
	}
      *treestr++ = ')';                    
      // write innernode as nodenum_b_nodenumback
#if 0
      sprintf(treestr, "%d", q->number);
      while (*treestr) treestr++;
      *treestr++ = 'b';                    
      sprintf(treestr, "%d", p->back->number);
      while (*treestr) treestr++;
#endif
    
    }

  if(q == tr->start->back) 
    {              
      if(printBranchLengths)
	sprintf(treestr, ":0.0;\n");
      else
	sprintf(treestr, ";\n");                  
    }
  else 
    {                   
      if(printBranchLengths)          
	{
	  //sprintf(treestr, ":%8.20f", getBranchLength(tr, SUMMARIZE_LH, p));                 
	  assert(tr->fracchange != -1.0);
	  z = q->z[0];
	  if (z < zmin) 
	    z = zmin;        
	  sprintf(treestr, ":%8.20f", -log(z) * tr->fracchange);               
	}
      else            
	sprintf(treestr, "%s", "\0");         
    }

  while (*treestr) treestr++;
  return  treestr;
}
#endif

/* static int tree_dump_num = 0; */






/* TODO we do not even use this function, do we? NOTE Now we do ;) */
void penalize(state *curstate, int which_proposal, int acceptance)
{
  double max=4.0;
  double min=0.0;
  if(acceptance){
    curstate->proposalLogisticT[which_proposal]+=curstate->penaltyFactor;
    if(curstate->proposalLogisticT[which_proposal]>max)
      curstate->proposalLogisticT[which_proposal]=max;
    //curstate->proposalWeights[which_proposal] /= curstate->penaltyFactor; 
  }
  else {
  /*  if(curstate->totalRejected>0){
    curstate->proposalLogisticT[which_proposal]-=((curstate->totalAccepted/curstate->totalRejected)*curstate->penaltyFactor);*///TODO check whether "VCG" is better than considering all
    if(curstate->totalRejected - curstate->rejectedProposals[which_proposal]>0){
    curstate->proposalLogisticT[which_proposal]-=(((curstate->totalAccepted- curstate->acceptedProposals[which_proposal])/(curstate->totalRejected - curstate->rejectedProposals[which_proposal]))*curstate->penaltyFactor);      
    }else{
      curstate->proposalLogisticT[which_proposal]-=curstate->penaltyFactor;
    }
    if(curstate->proposalLogisticT[which_proposal]<min)
      curstate->proposalLogisticT[which_proposal]=min;
    //curstate->proposalWeights[which_proposal] *= curstate->penaltyFactor; 
  }
  findLogisticP(curstate);
  //normalizeProposalWeights(curstate); 
}


void mcmc(tree *tr, analdef *adef)
{  
  int j;

  size_t inserts = 0;
  /* double printTime = 0; */
  
  //allocate states
  double bl_prior_exp_lambda = 0.1;
  double bl_sliding_window_w = 0.005;
  double gm_sliding_window_w = 0.75;
  double rt_sliding_window_w = 0.5;
  
  int sum_radius_accept = 0;
  int sum_radius_reject = 0;


  printf("initializing RNG with seed %d\n", seed); 

  initRNG(seed);
  
  tr->start = find_tip(tr->start, tr );
  
  FILE *log_h = fopen( "mcmc.txt", "w" );
  
  
  printf( "isTip: %d %d\n", tr->start->number, tr->mxtips );
  printf( "z: %f\n", tr->start->z[0] );
  
  assert( isTip(tr->start->number, tr->mxtips ));
  
  state *curstate = state_init(tr, adef,  bl_sliding_window_w, rt_sliding_window_w, gm_sliding_window_w, bl_prior_exp_lambda); 


#ifdef _USE_NCL_PARSER
  initParamStruct *initParams = NULL; 
  printf("\n\ntrying to parse %s\n\n", configFileName); 
  parseConfigWithNcl(configFileName, &initParams);   
  addInitParameters(curstate, initParams); 
#else  
  parseConfig(curstate); 
#endif
  normalizeProposalWeights(curstate); 
  

  int count = 0;
  traverse_branches_set_fixed( tr->start, &count, curstate, 0.65 );

  evaluateGeneric(tr, tr->start, TRUE);
  printf( "after reset start: %f\n", tr->likelihood );
  int first = 1;
  
  curstate->curprior = 1;
  curstate->hastings = 1;

#ifdef WITH_PERFORMANCE_MEASUREMENTS  
  perf_timer all_timer = perf_timer_make();
#endif
  
  /* beginning of the MCMC chain */
  for(j=0; j< curstate->numGen ; j++)
    {
#ifdef WITH_PERFORMANCE_MEASUREMENTS
      perf_timer move_timer = perf_timer_make();
#endif
      proposal_type which_proposal;
      /* double t = gettime();  */
      /* double proposalTime = 0.0; */
      double testr;
      double acceptance;
    
      //     printBothOpen("iter %d, tr LH %f, startLH %f\n",j, tr->likelihood, tr->startLH);
      /* proposalAccepted = FALSE; */
    
      //
      // start of the mcmc iteration
      //

    
      // just for validation (make sure we compare the same)
      evaluateGeneric(tr, tr->start, FALSE); 

#ifdef WITH_PERFORMANCE_MEASUREMENTS    
      perf_timer_add_int( &move_timer ); //////////////////////////////// ADD INT
#endif
      //      printBothOpen("before proposal, iter %d tr LH %f, startLH %f\n", j, tr->likelihood, tr->startLH);
      tr->startLH = tr->likelihood;

      //which_proposal = proposal(curstate);
    
      // select proposal type
      which_proposal = select_proposal_type( curstate );
    
      // apply the proposal function
      dispatch_proposal_apply(which_proposal, curstate );

#ifdef WITH_PERFORMANCE_MEASUREMENTS
      perf_timer_add_int( &move_timer ); //////////////////////////////// ADD INT
#endif
      // FIXME: why is this here?
      if (first == 1)
	{
	  first = 0;
	  curstate->curprior = curstate->newprior;
	}
      //     printBothOpen("proposal done, iter %d tr LH %f, startLH %f\n", j, tr->likelihood, tr->startLH);

      //proposalTime += gettime() - t;
      /* decide upon acceptance */
      testr = drawRandDouble();
      //should look something like 
      acceptance = fmin(1,(curstate->hastings) * 
			(exp(curstate->newprior-curstate->curprior)) * (exp(curstate->tr->likelihood-curstate->tr->startLH)));
    


#ifdef WITH_PERFORMANCE_MEASUREMENTS    
      perf_timer_add_int( &move_timer ); //////////////////////////////// ADD INT
#endif
      if(processID == 0 && (j % 100) == 0) 
	{
	  printf( "propb: %d %f %f %d spr: %d (%d) model: %d (%d) ga: %d (%d) bl: %d (%d) blExp: %d (%d) %f %f %f radius: %f %f\n", 
		  j, tr->likelihood, tr->startLH, testr < acceptance, 
		  curstate->acceptedProposals[SPR]	, curstate->rejectedProposals[SPR] , 
		  curstate->acceptedProposals[UPDATE_MODEL]	, curstate->rejectedProposals[UPDATE_MODEL] , 
		  curstate->acceptedProposals[UPDATE_GAMMA]	, curstate->rejectedProposals[UPDATE_GAMMA] , 
		  curstate->acceptedProposals[UPDATE_SINGLE_BL], curstate->rejectedProposals[UPDATE_SINGLE_BL], 
		  curstate->acceptedProposals[UPDATE_SINGLE_BL_EXP], curstate->rejectedProposals[UPDATE_SINGLE_BL_EXP],
		  curstate->hastings, curstate->newprior, curstate->curprior, 
		  sum_radius_accept / (float)curstate->acceptedProposals[SPR], 
		  sum_radius_reject / (float)curstate->rejectedProposals[SPR] );
      
	  printSubsRates(curstate->tr, curstate->modelRemem.model, curstate->modelRemem.numSubsRates);

#ifdef WITH_PERFORMANCE_MEASUREMENTS      
	  perf_timer_print( &all_timer );
#endif
      
      
	  fprintf( log_h, "%d %f %f\n", j, tr->likelihood, tr->startLH );
	}

      assert(which_proposal < NUM_PROPOSALS); 
      
      if(testr < acceptance)
	{
	  /* proposalAccepted = TRUE; */

	  curstate->acceptedProposals[which_proposal]++; 
	  curstate->totalAccepted++;
	 // curstate->proposalWeights[which_proposal] /= curstate->penaltyFactor;
	  penalize(curstate, which_proposal, 1);
	  if(which_proposal == SPR)
	    sum_radius_accept += spr_depth; 

	  curstate->tr->startLH = curstate->tr->likelihood;  //new LH
	  curstate->curprior = curstate->newprior;          
	}
      else
	{
	  dispatch_proposal_reset(which_proposal,curstate);
	  curstate->rejectedProposals[which_proposal]++;
	  curstate->totalRejected++;
	  //curstate->proposalWeights[which_proposal] *= curstate->penaltyFactor; 
	  penalize(curstate, which_proposal, 0);
	  if(which_proposal == SPR)
	    sum_radius_reject += spr_depth;
      
      
	  evaluateGeneric(tr, tr->start, FALSE); 

	  // just for validation 
	  if(fabs(curstate->tr->startLH - tr->likelihood) > 1.0E-15)
	    {
	      printBothOpen("WARNING: LH diff %.20f\n", curstate->tr->startLH - tr->likelihood);
	      printBothOpen("after reset, iter %d tr LH %f, startLH %f\n", j, tr->likelihood, tr->startLH);
	    }      
	  assert(fabs(curstate->tr->startLH - tr->likelihood) < 0.1);
	} 

      inserts++;
#ifdef WITH_PERFORMANCE_MEASUREMENTS
      perf_timer_add_int( &move_timer ); //////////////////////////////// ADD INT    
      perf_timer_add( &all_timer, &move_timer );
#endif

    }
}


/****************************************************************************************/
/* garbage : this is code that was not used in the initial version, but may be of use   */
/****************************************************************************************/


/* let's decide at some point, if we want to use anything of that */

#if 0 
static void print_state_file(int iter, state * curstate)
{ 
  const size_t tmp_len = 256;
  char tmp[tmp_len];
  
  strncpy(tmp, "RAxML_states.", tmp_len);
  strncat(tmp, run_id, tmp_len);
  FILE *f = myfopen(tmp, "ab");
  fprintf(f,"%d\t%f",iter, curstate->tr->likelihood);
  int i;
  for(i = 0;i < curstate->modelRemem.numSubsRates; i++)
    {
      fprintf(f,"\t%f",curstate->modelRemem.curSubsRates[i]);
    }
  fprintf(f,"\t%f",curstate->gammaRemem.curAlpha);
  fprintf(f,"\n");
  fclose(f);
}

static void update_all_branches(state * s, boolean resetBL)
{
  int updated_branches = 0;
  assert(isTip(s->tr->start->number, s->tr->mxtips));
  /* visit each branch exactly once */
  traverse_branches(s->tr->start->back, &updated_branches, s, resetBL);
  assert(updated_branches == s->tr->mxtips + s->tr->mxtips - 3);
}


static void traverse_branches(nodeptr p, int *count, state * s, boolean resetBL)
{
  nodeptr q;
  //printf("current BL at %db%d: %f\n", p->number, p->back->number, p->z[0]);
  if(resetBL)
    reset_branch_length(p, s->tr->numBranches);
  else//can allow for other methods later
    set_branch_length_sliding_window(p, s->tr->numBranches, s, TRUE);
  *count += 1;


  if (! isTip(p->number, s->tr->mxtips)) 
    {                                  /*  Adjust descendants */
      q = p->next;
      while (q != p) 
	{
	  traverse_branches(q->back, count, s, resetBL);
	  q = q->next;
	}   
      // WTF? in each recursion?
      newviewGeneric(s->tr, p, FALSE);     // not sure if we need this
    }
}


static void printRecomTree(tree *tr, boolean printBranchLengths, char *title)
{
  FILE *nwfile;
  const size_t tmp_len = 256;
  char tmp[tmp_len];
  
  snprintf( tmp, tmp_len, "spr_%04d.txt", tree_dump_num );
  ++tree_dump_num;
  
  nwfile = myfopen(tmp, "w");
  Tree2StringRecomREC(tr->tree_string, tr, tr->start->back, printBranchLengths);
  fprintf(nwfile,"%s\n", tr->tree_string);
  fclose(nwfile);
  if(title)
    printBothOpen("%s\n", title);
  if (printBranchLengths)
    printBothOpen("%s\n", tr->tree_string);
  printBothOpen("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
  //system("bin/nw_display tmp.nw");


}  




/*
 * should be sliding window proposal
 */

static boolean simpleBranchLengthProposalApply(state * instate)
{
   
  //for each branch get the current branch length
  //pull a uniform like
  //x = current, w =window
  //uniform(x-w/2,x+w/2)

  update_all_branches(instate, FALSE);
  evaluateGeneric(instate->tr, instate->tr->start, TRUE); /* update the tr->likelihood */

  //for prior, just using exponential for now
  //calculate for each branch length
  // where lambda is chosen and x is the branch length
  //lambda * exp(-lamba * x)

  //only calculate the new ones
  //
  return TRUE;
}

static void simpleBranchLengthProposalReset(state * instate)
{
  update_all_branches(instate, TRUE);
}



#endif

