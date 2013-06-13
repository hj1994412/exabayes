#ifndef _TREEALN_H
#define _TREEALN_H

#include <vector>
#include <iostream>
#include <memory>

using namespace std; 

#include "Priors.hpp"
#include "branch.h"
#include "axml.h"

#include "TreePrinter.hpp"

static int numStateToNumInTriangleMatrix(int numStates) 
{  
  return (  numStates * numStates - numStates) / 2 ; 
}

class LnlRestorer; 
class Partition; 

/** 
    @brief mostly wraps the legacy tr and partition list     
*/

class TreeAln
{
public: 
  explicit TreeAln();
  ~TreeAln();
  TreeAln& operator=( TreeAln &rhs); 
  
  void initRevMat(int model); 
  void unlinkTree();
  nodeptr getUnhookedNode(int number);
  void discretizeGamma(int model); 

  void initializeFromByteFile(string  byteFileName); 
  
  void enableParsimony();


  // BEGIN collapse test 
  void collapseBranch(branch b); 
  bool isCollapsed(branch b) ; 
  void setBranchLengthUnsafe(branch b ) ; 
  // END
  


  // TODO 
  // string toString(bool showInternal, bool showBL, bool printNames ) const ; 


  
  /** 
      @brief clips two nodes together. Notice that z is a reference to
      the internal branch length. If z is outside the bl-bounds, it
      will yield a corrected z this way.
   */ 
  void clipNode(nodeptr p, nodeptr q, double &z); 
  /**
     @brief clips two nodes with a default branch length
   */ 
  void clipNodeDefault(nodeptr p, nodeptr q); 

#if 0 
  /**
     @brief gets the tree length (real value)
  */   
  double getTreeLength() const ; 
#endif

  // these are not your average setters! They modify the original value
  void setFrequenciesBounded(vector<double> &newValues, int model ); 
  void setRevMatBounded(vector<double> &newValues, int model); 
  void setBranchLengthBounded(double &newValue, int model, nodeptr p); 
  void setAlphaBounded(double &newValue, int model); 
  
  void setTr(tree *newTr){ tr = newTr; }
#if HAVE_PLL != 0 
  void setPartitionList(partitionList *pl) { partitions = pl; }
#endif
  

  // getters 
  pInfo* getPartition(int model) const;
  tree* getTr() {return tr;}
  const tree* getTr() const{return tr; }
  int getNumBranches() const; 
  int getNumberOfPartitions() const;   
  boolean& accessExecModel(int model); 
  double& accessPartitionLH(int model); 

  // getters for various parameters. We should use them to reduce the
  // direct acceses to for instance branch lengths. Makes our code
  // saver and if we want to do something globally (e.g., print), then
  // it can simply be added to these functions.
  double getBranchLength(nodeptr p, int model) const {return p->z[model] ;  }

  vector<double> getRevMat(int model) const ; 
  vector<double> getFrequencies(int model) const; 

  double getAlpha(int model) const {pInfo *partition = getPartition(model) ; return partition->alpha; } 
  bool isTipNode(nodeptr p) const {return isTip(p->number, getTr()->mxtips );}

#if HAVE_PLL != 0
  partitionList* getPartitionsPtr() const { return partitions; } 
#endif


  // those are the constraints for various parameters 
  static const double zMin, zMax, 
    rateMax, rateMin, 
    alphaMin, alphaMax,
    freqMin; 

  static const double zZero;   
  static const double initBL;  	// init values 

  friend ostream& operator<< (ostream& out,  TreeAln&  traln);

  void setRestorer(shared_ptr<LnlRestorer> rest){ restorer = rest; }
  auto getRestorer() const ->  shared_ptr<LnlRestorer> {return restorer;  }  
  
  /** @brief the partition does not have a revmat */ 
  bool revMatIsImmutable(int model) const ; 

  double getTreeLengthExpensive() const;

  void verifyTreeLength() const; 

  double getConvertBranchLength(double length) const { return -log(length) * tr->fracchange; }

private:   
  double getTreeLengthHelper(nodeptr p) const;
  
  void initDefault();
  tree* tr;		// TODO replace with an object for cleanup 
  
  bool parsimonyEnabled;   
  shared_ptr<LnlRestorer> restorer; 

#if HAVE_PLL != 0
  // horrible hacks, that we cannot get rid of before  upgrading to more recent versions of the PLL 
  partitionList* partitions; 
  void initializeTreePLL(string byteFileName);
  void initializePartitionsPLL(string byteFileName, double ***empFreq, bool multiBranch);
#endif  

};  


#endif
