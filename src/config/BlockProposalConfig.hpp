#ifndef _BLOCK_PROPOSALCONFIG_H
#define _BLOCK_PROPOSALCONFIG_H

#include <cassert>
#include <ncl/ncl.h>
#include <map>

#include "ProposalType.hpp"
#include "GlobalVariables.hpp"

class BlockProposalConfig : public NxsBlock
{
public: 
  BlockProposalConfig();
  virtual void Read(NxsToken &token); 

  bool wasSetByUser(ProposalType type ) const {  return userValue.find(type) != userValue.end() ; }
  double getProposalWeight(ProposalType type) const
  { 
    assert(userValue.find(type) != userValue.end()) ; return userValue.at(type); 
  }

  double getEsprStopProp() const {return esprStopProp; } 
  double getEtbrStopProb() const {return etbrStopProb; }
  double getParsimonyWarp() const {return parsimonyWarp; }
  int getGuidedRadius() const {return guidedRadius; }  
  void verify(); 

private: 
  std::unordered_map<ProposalType, double, ProposalTypeHash> userValue; 

  double etbrStopProb; 
  double esprStopProp; 
  double parsimonyWarp;   
  int guidedRadius; 
}; 


#endif
