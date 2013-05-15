#ifndef _EXTENDED_SPR_H
#define _EXTENDED_SPR_H

#include "axml.h"
#include "AbstractProposal.hpp"
#include "Randomness.hpp"
#include "Path.hpp"


class ExtendedSPR : public AbstractProposal
{
public: 
  ExtendedSPR( double relativeWeight, double stopProb, double multiplier); 
  virtual ~ExtendedSPR(); 

  virtual void applyToState(TreeAln &traln, PriorBelief &prior, double &hastings, Randomness &rand) ; 
  virtual void evaluateProposal(TreeAln &traln, PriorBelief &prior) ; 
  virtual void resetState(TreeAln &traln, PriorBelief &prior) ; 

  virtual void autotune() {}	// disabled 

  virtual AbstractProposal* clone() const;  

protected: 
  double stopProb; 
  double multiplier; 
  Path modifiedPath; 

  void destroyOrientationAlongPath( Path& path, tree *tr,  nodeptr p); 
  void drawPathForESPR( TreeAln& traln, Randomness &rand, double stopProp ); 
  void multiplyAlongBranchESPR(TreeAln &traln, Randomness &rand, double &hastings, PriorBelief &prior ); 
  void applyPathAsESPR(TreeAln &traln ); 
  void resetAlongPathForESPR(TreeAln &traln, PriorBelief &prior); 

}; 


#endif
