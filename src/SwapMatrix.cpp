#include "SwapMatrix.hpp"

#include "GlobalVariables.hpp"

#include <iostream>
#include <cassert>

SwapMatrix::SwapMatrix(nat numChains)
  : numEntries(numChains)
{
  for(nat i = 0; i < numEntries-1; ++i)
    for(nat j = i  + 1 ; j < numEntries; ++j)
      matrix.push_back(SuccessCounter()); 

}


void SwapMatrix::update(nat a, nat b, bool acc)
{
  auto &elem = matrix[mapToIndex(a,b)];
  if(acc)
    elem.accept();
  else 
    elem.reject();
}


const SuccessCounter& SwapMatrix::getCounter(nat a, nat b ) const
{
  return matrix[mapToIndex(a,b)]; 
} 



nat SwapMatrix::mapToIndex(nat a, nat b) const 
{
  if(b < a )
    std::swap(a,b); 

  nat result = 0; 
  for(nat i = 0; i < a && a != 0; ++i)
    result += numEntries -i  -1 ; 
  result += b - a - 1  ; 

  assert(result < matrix.size()); 

  return result; 
} 


std::ostream&  operator<<(std::ostream &out, SwapMatrix& rhs ) 
{
  for(nat i = 0; i < rhs.numEntries-1; ++i)
    {
      tout << "(" ; 
      bool isFirst = true; 
      for(nat j = i+1; j < rhs.numEntries; ++j)
	{
	  auto &elem = rhs.matrix[rhs.mapToIndex(i,j)]; 
	  out << (isFirst ? "" : ",") << std::setprecision(1) << 100 * elem.getRatioOverall() << "%"; 
	  isFirst = false; 
	}
      tout << ")"; 
    }
  return out ; 
}


void SwapMatrix::readFromCheckpoint( std::ifstream &in )   
{  
  for(auto &s : matrix)
    s.readFromCheckpoint(in);
} 

void SwapMatrix::writeToCheckpoint( std::ofstream &out) const
{
  for(auto &s : matrix)
    s.writeToCheckpoint(out); 
}
   