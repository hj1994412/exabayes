#ifndef _BRANCHLENGTHSMULTIPLIER_H
#define _BRANCHLENGTHSMULTIPLIER_H

#include "AbstractProposal.hpp"



class BranchLengthMultiplier : public AbstractProposal
{
public: 
  BranchLengthMultiplier(  double multiplier); 
  virtual ~BranchLengthMultiplier(){}

  virtual void applyToState(TreeAln &traln, PriorBelief &prior, double &hastings, Randomness &rand) ; 
  virtual void evaluateProposal(LikelihoodEvaluatorPtr &evaluator,TreeAln &traln, PriorBelief &prior) ; 
  virtual void resetState(TreeAln &traln, PriorBelief &prior) ; 

  virtual void autotune();

  virtual AbstractProposal* clone() const;  


  virtual Branch proposeBranch(const TreeAln &traln, Randomness &rand) const ;   

protected: 
  double multiplier;  

private: 
  
  Branch savedBranch; 
}; 


#endif
