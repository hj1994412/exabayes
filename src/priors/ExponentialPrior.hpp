#ifndef _EXPONENTIAL_PRIOR 
#define _EXPONENTIAL_PRIOR 

#include "AbstractPrior.hpp"
#include "Density.hpp"
 
class ExponentialPrior : public AbstractPrior
{
public: 
  ExponentialPrior(double lambda) ; 
  virtual double getLogProb(std::vector<double> values) const  ; 
  virtual std::vector<double> drawFromPrior(Randomness &rand)  const; 
  virtual void print(std::ostream& out ) const  ; 
  virtual double getLamda()  const  { return lambda; } 
  virtual ParameterContent getInitialValue() const; 
  virtual double accountForMeanSubstChange( TreeAln &traln, const AbstractParameter* param, double myOld, double myNew )  const ; 

private: 
  double lambda; 
}; 

#endif
