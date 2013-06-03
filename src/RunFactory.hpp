/**
   @file RunFactory

   @brief an essential class that uses all available information to
   create the exact setup of the run in terms of priors, proposals and
   so on


*/ 


#ifndef _PROPOSALFACTORY
#define _PROPOSALFACTORY

#include <vector>

#include "BlockProposalConfig.hpp"
#include "BlockPrior.hpp"
#include "BlockParams.hpp"
#include "AbstractProposal.hpp"
#include "TreeAln.hpp"
#include "GlobalVariables.hpp"
#include "RandomVariable.hpp"


class RunFactory
{
public: 
  RunFactory(){}

  void configureRuns(const BlockProposalConfig &propConfig, const BlockPrior &priorInfo, const BlockParams& partitionParams, const TreeAln &traln, vector<unique_ptr<AbstractProposal> > &proposalResult); 
  
  vector<RandomVariable> getRandomVariables() const {return randomVariables; }

private: 
  void addStandardParameters(vector<RandomVariable> &vars, const TreeAln &traln ); 
  void addPriorsToVariables(const TreeAln &traln,  const BlockPrior &priorInfo, vector<RandomVariable> &variables); 
  void addStandardPrior(RandomVariable &var, const TreeAln& traln ); 

  // DUMMY
  // void setupProposals(vector<Category> &proposalCategories, vector<double> proposalWeights, const PriorBelief &prior); 

  // vector<unique_ptr<AbstractProposal> > proposals; 
  vector<RandomVariable> randomVariables; 

}; 

#endif