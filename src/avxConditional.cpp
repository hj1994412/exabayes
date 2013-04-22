#include "config.h" 


#ifdef HAVE_AVX

#define  __AVX

#if HAVE_PLL != 0
#include "pll/avxLikelihood-pll.c"
#else 
#include "examl/avxLikelihood.c"
#endif
#else 


static int prototypeDummy()
{
  return 0; 
}

#endif
