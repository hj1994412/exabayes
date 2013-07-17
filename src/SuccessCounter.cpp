#include <iostream>
#include <cassert>
#include "SuccessCounter.hpp"

SuccessCounter::SuccessCounter() 
  : globalAcc(0),globalRej(0)
  , localAcc(0), localRej(0), batch(0)
{
}


void SuccessCounter::accept() 
{
  globalAcc++;
  localAcc++; 
  addOrReplace(true); 
}


void SuccessCounter::reject()
{
  globalRej++;
  localRej++; 
  addOrReplace(false); 
}


double SuccessCounter::getRatioInLast100() const 
{
  double acc = 0, rej = 0 ; 
  for(auto b : lastX )
    if(b)
      acc++;
    else 
      rej++; 
  return acc / (acc+rej) ; 
}



double SuccessCounter::getRatioInLastInterval() const 
{ 
  double ratio =   ((double)localAcc) / ((double)(localAcc + localRej) ); 
  // cout << "ratio is " <<  ratio << endl; 
  return  ratio ; 
}


double SuccessCounter::getRatioOverall() const 
{
  return (double) globalAcc / ((double)(globalAcc + globalRej)); 
}

void SuccessCounter::nextBatch()
{
  batch++;
  reset();
}



void SuccessCounter::reset()
{
  localRej = 0; localAcc = 0; 
}


void SuccessCounter::addOrReplace(bool acc)
{ 
  if(lastX.size() ==  SIZE_OF_LAST) 
    {
      lastX.pop_front(); 
    }
  lastX.push_back(acc); 
}


void SuccessCounter::readFromCheckpoint( std::ifstream &in )   
{
  in >> globalAcc; 
  readDelimiter(in); 
  in >> globalRej; 
  readDelimiter(in); 
  in >> localAcc; 
  readDelimiter(in); 
  in >> localRej; 
  readDelimiter(in); 
  in >> batch; 
  readDelimiter(in); 
}
 
void SuccessCounter::writeToCheckpoint( std::ofstream &out) 
{
  out << globalAcc << DELIM
      << globalRej << DELIM
      << localAcc << DELIM 
      << localRej << DELIM 
      << batch << DELIM ; 
} 
