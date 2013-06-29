#include "GibbsProposal.hpp" 
#include "BranchLengthMultiplier.hpp"

class GibbsBranchLength : public BranchLengthMultiplier
{
public: 
  GibbsBranchLength(LikelihoodEvaluatorPtr eval)
    : BranchLengthMultiplier(0)
  {
    name = "estGibbsBL"; 
    category = Category::BRANCH_LENGTHS; 
    relativeWeight = 0 ; 
  } 


  virtual void applyToState(TreeAln &traln, PriorBelief &prior, double &hastings, Randomness &rand) 
  {
    Branch  b = proposeBranch(traln, rand);     
    b.setLength(b.findNodePtr(traln)->z[0]); 
    double initBl = b.getLength(); 

    nodeptr p = b.findNodePtr(traln); 
    savedBranch = b;     
    double newInterpretedLength = GibbsProposal::drawFromEsitmatedPosterior(b, traln, rand, 0.1, 5, hastings); 

    double newZ = b.getInternalLength(traln, newInterpretedLength); 
    
    auto brPr = primVar[0]->getPrior();
    prior.updateBranchLengthPrior(traln,initBl, newZ, brPr);
  }


  virtual ~GibbsBranchLength(){}  
  virtual void autotune(){}

  virtual AbstractProposal* clone() const  { return new GibbsBranchLength(*this); }  
  
private: 
  LikelihoodEvaluatorPtr eval; 

}; 
