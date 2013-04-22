/**
   @file  topology-utils.h
   
   @brief Various functions for manipulation of topologies and branch lengths. 
*/ 

#ifndef _UTILS_TOPO_H
#define _UTILS_TOPO_H

#include "path.h"

void insertWithGenericBL (nodeptr insertNode, nodeptr branchNode, double *insertZ, double *branchNodeZ, double *neighbourZ ,  int numBranches);
void insertWithUnifBL (nodeptr insertNode, nodeptr branchNode, int numBranches);
void insertWithUnifBLScaled(nodeptr insertNode, nodeptr branchNode, double scale, int numBranches);

void traverseAndCount(nodeptr p, int *count, tree *tr ); 

void applyPathAsESPR(TreeAln *traln, path *rPath ); 
void destroyOrientationAlongPath(tree *tr, path *rPath, nodeptr p); 
void resetAlongPathForESPR(TreeAln *traln, path *rPath); 
double getTreeLength(TreeAln *traln, nodeptr p); 

void restoreBranchLengthsPath(TreeAln *traln, path *s); 
#endif
