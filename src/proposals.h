/**
   @file proposals.h

   @brief All proposals that make the chains of ExaBayes move in its
   space.
    
 */ 

#ifndef _PROPOSALS_H
#define _PROPOSALS_H

#include "proposalFunction.h"
#include "ConfigReader.hpp"


void initProposalFunction( proposal_type type, vector<double> weights, proposalFunction **result);
double tuneParameter(int batch, double accRatio, double parameter, boolean inverse ); 
#endif
