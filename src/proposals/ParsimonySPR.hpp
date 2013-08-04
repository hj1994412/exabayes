/** 
    @file ParsimonySPR.hpp
    
    @brief implements a parsimony-biased SPR move similar to MrBayes. 

    @notice: MrBayes also reweights site patters -- currently we do
    not do that.
 */ 

#ifndef __PARSIMONY_SPR
#define __PARSIMONY_SPR



// why not evaluate stuff only on a few partitions and use that for guidance? 


#include <unordered_map>

#include "axml.h"
#include "AbstractProposal.hpp"
#include "Path.hpp"
#include "SprMove.hpp"
#include "ParsimonyEvaluator.hpp"

typedef std::unordered_map<Branch, double, BranchHashNoLength, BranchEqualNoLength> weightMap; 
typedef std::unordered_map<Branch,std::vector<nat>, BranchHashNoLength, BranchEqualNoLength> scoreMap; 

class ParsimonySPR : public AbstractProposal
{
public: 
  ParsimonySPR( double parsWarp, double blMulti); 
  virtual ~ParsimonySPR(){}

  virtual void applyToState(TreeAln &traln, PriorBelief &prior, double &hastings, Randomness &rand) ; 
  virtual void evaluateProposal(  LikelihoodEvaluator *evaluator, TreeAln &traln, PriorBelief &prior) ; 
  virtual void resetState(TreeAln &traln, PriorBelief &prior) ; 
  virtual void autotune() ;
  
  // virtual Branch prepareForSetExecution(TreeAln &traln, Randomness &rand)  { return Branch(0,0);}
  virtual std::pair<Branch,Branch> prepareForSetExecution(TreeAln &traln, Randomness &rand)  { return std::pair<Branch, Branch> (Branch(0,0),Branch(0,0) );}

  AbstractProposal* clone() const; 

  virtual void readFromCheckpointCore(std::istream &in) {   } // disabled
  virtual void writeToCheckpointCore(std::ostream &out) const { } //disabled

protected: 
  double parsWarp; 
  double blMulti;   

  weightMap getWeights(const TreeAln& traln, const scoreMap &insertions) const; 
  void determineSprPath(TreeAln& traln, Randomness &rand, double &hastings, PriorBelief &prior ); 
  void traverse(const TreeAln &traln, nodeptr p, int distance ); 
  void testInsertParsimony(TreeAln &traln, nodeptr insertPos, nodeptr prunedTree, std::unordered_map<Branch,std::vector<nat>, BranchHashNoLength, BranchEqualNoLength > &posses); 
  
  SprMove move; 
  ParsimonyEvaluator pEval;   
}; 


#endif
