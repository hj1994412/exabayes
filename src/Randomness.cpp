#include <limits>

#include "Randomness.hpp" 
#include "densities.h"

// TODO proper AND carefull make-over of randomness


Randomness::Randomness(int seed)
{
  key.v[0] = seed; 
  key.v[1] = 0; 
  ctr.v[0] = 0; 
  ctr.v[1] = 0; 
}

randCtr_t Randomness::generateSeed()
{
  randCtr_t r =  exa_rand(ctr, key);   
  incrementNoLimit();
  return r; 
}




void Randomness::incrementNoLimit()
{
  ctr.v[0]++; 
  if(ctr.v[0] == std::numeric_limits<unsigned int>::max())
    {
      ctr.v[1]++; 
      ctr.v[0] = 0;     
    }
}




/** 
    @brief draw integer uniformly from [0,upperBound]
 */ 
int Randomness::drawIntegerClosed(int upperBound)
{
  randCtr_t r = exa_rand(key, ctr); 
  ctr.v[1]++;
  return r.v[0] % (upperBound + 1 ) ; 
}


/** 
    @brief for transition: draw integer uniformly from [0,upperBound)
 */ 
int Randomness::drawIntegerOpen(int upperBound)
{
  assert(upperBound > 1); 
  return drawIntegerClosed(upperBound-1);   
}



/** 
    @brief draw integer uniformly from [0,upperBound)
 */ 
int Randomness::drawRandInt(int upperBound )
{
  randCtr_t r = exa_rand(key, ctr); 
  ctr.v[1]++;   
  return r.v[0] % upperBound; 
}


double Randomness::drawRandDouble01()
{
  randCtr_t r = exa_rand(key, ctr); 
  ctr.v[1]++; 
  return u01_closed_open_32_53(r.v[1]); 
}


double Randomness::drawRandExp(double lambda)
{  
  double r = drawRandDouble01();   
  return -log(r )/ lambda; 
}



double Randomness::drawRandBiUnif(double x)
{
  double r = drawRandDouble01() *  (2*x-x/2) + x / (3/2) ; 
  return r; 
}




/** @brief gets a multiplier for updating a parameter or branch length */
double Randomness::drawMultiplier(double multiplier) // 
{

  double tmp =  exp(multiplier * (drawRandDouble01()  - 0.5)); 
  assert(tmp > 0.); 
  return tmp ;   
}


// alternative slider as used in mrBayes
double Randomness::drawFromSlidingWindow(double value, double window)
{
  return value + window * (drawRandDouble01() - 0.5); 
}


//get random permutation of [0,n-1]
void Randomness::drawPermutation( int* perm, int n)
{
  int i;
  int randomNumber;
  perm[0] = 0;
  
  for(i=1 ; i<n ; i++){
  
    randomNumber = drawRandInt(i+1);
    // randomNumber=rand() % (i+1);
    // randomNumber=rand();

    if(randomNumber==i){
      perm[i]=i;
    }else{
      perm[i]=perm[randomNumber];
      perm[randomNumber]=i;
    }
  }

}





/**
   @brief draw r according to distribution given by weights. 

   NOTE sum of weights is not required to be 1.0
*/
int Randomness::drawSampleProportionally( double *weights, int numWeight )
{
  double r = drawRandDouble01();
 
  double sum=0.0;
  double lower_bound = 0.0;
  int i = 0; 
  
  assert( numWeight > 0 );
  
  for(  i = 0; i < numWeight ; ++i ) 
    {
      sum+=weights[i]; 
    }
  assert(sum>0);
  r=r*sum;
    
  for( int i = 0; i < numWeight ; ++i ) 
    {
      double upper_bound = lower_bound + weights[i];
    
      if( r >= lower_bound && r < upper_bound ) 
	return i ;
    
      lower_bound = upper_bound; 
    }
  
  return i-1;
}











//This function should be called if the alphas for the dirichlet distribution are given
void Randomness::drawRandDirichlet( vector<double> &results, const vector<double> &alphas)
{
  double sum=0;
  results.resize(alphas.size()); 
  for(nat i=0; i< alphas.size();i++)
    {
      results[i] = drawRandGamma(alphas[i], 1.0) ;
      sum += results[i];
    }
  for(nat i=0; i< alphas.size();i++)
    results[i] /= sum;
}



//This function should be called if the expected values for the dirichlet distribution are given
void Randomness::drawDirichletExpected(vector<double> &results, const vector<double> &mean,double scale)
{
  vector<double> alphas; 
  double originalSum=0;

  for(nat i=0; i< mean.size();i++)
    {
      originalSum+=mean[i];
      alphas.push_back( mean[i]*scale ) ;
    }
  
  drawRandDirichlet( results, alphas);

  for(nat i=0; i< alphas.size() ;i++)
    results[i]=results[i]*originalSum;      
}


// Gamma(alpha, beta) sampling
double Randomness::drawRandGamma(double alpha, double beta)
{

  double alpha_min=0.0001;
  if(alpha<alpha_min)
    alpha=alpha_min;
  
  if(beta<0)
    beta=0;
  
  double gamma=0;
   
  int escape=0, limit=2000;
  bool alert=false;
   
  if (alpha < 1) {
    double r;
    boolean done=false;
    double b=1+1/alpha;
    while(!done) {
      r=drawRandDouble01();
      if (r>1/b) {
	gamma=-log((b-r)/alpha);
	if (drawRandDouble01()<=pow(gamma,alpha-1)) 
	  done=true;
      }
      else {
	gamma=pow(r,1/alpha);
	if (drawRandDouble01()<=exp(-gamma)) 
	  done=true;
      }
        
        
        
      escape++;
      if(alert && escape>limit)
	{
	  printf("r>1/b==%.2f>%.2f\n",r,1/b);
	  printf("FREQ_MIN==%f alpha==%f gamma==%.2f\n",FREQ_MIN, alpha, gamma);
	  alert=false; 
	  //assert(0);
	}
	
	
	
    }
  } 
  else if (alpha == 1) {//GAMMA(1,1)=EXP(1)
    gamma = -log (drawRandDouble01());
  } else {    
    double y = -log (drawRandDouble01());
    while (drawRandDouble01() > pow (y * exp (1 - y), alpha - 1)){
      y = -log (drawRandDouble01());
      escape++;
      if(alert && escape>limit){
	printf("in second\n");
	alert=false; 
      }
    }
    gamma = alpha * y;
  }
  return beta*gamma;//scale from GAMMA(alpha,1) to GAMMA(alpha,beta)
}



ostream& operator<<(ostream& out, const Randomness &rhs)
{
  out << "key={" << rhs.key.v[0] << "," << rhs.key.v[1] << "},ctr={" << rhs.ctr.v[0] << ","<< rhs.ctr.v[1] << "}"; 
  return out; 
} 
