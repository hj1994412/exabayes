/** 
    @file SampleMaster.hpp
    
    @brief represents various chains sampling the posterior probability space
    
    Despite of its modest name, this is in fact the master class.  
 */ 

#ifndef _SAMPLING_H
#define _SAMPLING_H

#include <vector>

#include "ProposalSet.hpp"
#include "axml.h"
#include "config/BlockRunParameters.hpp"
#include "config/BlockProposalConfig.hpp"
#include "config/CommandLine.hpp"
#include "CoupledChains.hpp"
#include "config/ConfigReader.hpp"
#include "ParallelSetup.hpp"
#include "time.hpp"
#include "Checkpointable.hpp"
#include "file/DiagnosticsFile.hpp"


class SampleMaster : public Checkpointable
{
public:   
  SampleMaster(const ParallelSetup &pl, const CommandLine& cl ) ; 
  /** 
      @brief initializes the runs  
      @notice this is the top-level function 
   */ 
  void initializeRuns( ); 
  /** 
      @brief cleanup, once finished
   */ 
  void finalizeRuns();  
  /** 
      @brief starts the MCMC sampling   
   */ 
  void run(); 
  /** 
      @brief initializes the config file 
   */ 
  void processConfigFile(string configFileName, const TreeAln* tralnPtr, vector<unique_ptr<AbstractProposal> > &proposalResult, 
			  vector<unique_ptr<AbstractParameter> > &variableResult, std::vector<ProposalSet> &proposalSets, 
			  const std::unique_ptr<LikelihoodEvaluator> &eval); 
  void initializeWithParamInitValues(std::vector<shared_ptr<TreeAln>> &trees , const std::vector<AbstractParameter*> &params ) const ; 
  /** 
      @brief EXPERIMENTAL 
   */ 
  void branchLengthsIntegration()  ;  
  /** 
      @brief print information about the alignment  
   */ 
  void printAlignmentInfo(const TreeAln &traln);   
  /** 
      @brief gets the runs 
   */ 
  const std::vector<CoupledChains>& getRuns() const {return runs; }
  
  virtual void readFromCheckpoint( std::istream &in ) ; 
  virtual void writeToCheckpoint( std::ostream &out) const ;

private: 
  void printParameters(const TreeAln &traln, const std::vector<unique_ptr<AbstractParameter> > &params) const; 
  void printProposals(const std::vector<unique_ptr<AbstractProposal> > &proposals, const std::vector<ProposalSet> &proposalSets  ) const ; 
  void printInitializedFiles() const; 
  std::vector<std::string> getStartingTreeStrings(); 
  void informPrint(); 
  void printInitialState(const ParallelSetup &pl); 
  std::unique_ptr<LikelihoodEvaluator> createEvaluatorPrototype(const TreeAln &initTree); 
  void writeCheckpointMaster(); 
  void initializeFromCheckpoint(); 
  std::pair<double,double> convergenceDiagnostic(nat &begin, nat &end); 
  void initTrees(vector<shared_ptr<TreeAln> > &trees, randCtr_t seed, nat &treesConsumed, std::vector<std::string> startingTreeStrings, const std::vector<AbstractParameter*> &params); 
  void initializeTree(TreeAln &traln, std::string startingTree, Randomness &treeRandomness, const std::vector<AbstractParameter*> &params); 
  CLOCK::system_clock::time_point printDuringRun(nat gen,  ParallelSetup &pl) ; 

private:			// ATTRIBUTES 
  vector<CoupledChains> runs; 
  ParallelSetup pl; 
  CLOCK::system_clock::time_point initTime; 
  BlockParams paramBlock; 
  BlockRunParameters runParams;  
  BlockProposalConfig propConfig;   
  CommandLine cl; 
  CLOCK::system_clock::time_point lastPrintTime; 
  DiagnosticsFile diagFile; 
  Randomness masterRand;   	// not checkpointed
};  

#endif
