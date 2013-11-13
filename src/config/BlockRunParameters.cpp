#include "BlockRunParameters.hpp" 


BlockRunParameters::BlockRunParameters()  
  : diagFreq(5000) 
  , asdsfIgnoreFreq(0.1)
  , asdsfConvergence (0.001)
  , burninGen(0)
  , burninProportion(0.25)
  , samplingFreq (500)
  , numRunConv(1)
  , numGen(1000000)
  , numCoupledChains(1)
  , printFreq (500)
  , heatFactor(0.1)
  , swapInterval (1)
  , tuneHeat (false)
  , tuneFreq (100)
  , useParsimonyStarting(false)
  , heatedChainsUseSame(false)
  , chkpntFreq(1000)
  , componentWiseMH(true)
  , useAsdsfMax(false)
  , numSwaps(1)
{
  NCL_BLOCKTYPE_ATTR_NAME = "runconfig"; 
}


static bool convertToBool(NxsString &string)
{
  if(string.EqualsCaseInsensitive("true"))
    return true ; 
  else if (string.EqualsCaseInsensitive("false"))
    return false; 
  else 
    {
      cerr << "ERROR while parsing boolean value: expected either \"true\" or \"false\"" << endl; 
      assert(0); 
      return false; 
    }  
}


void BlockRunParameters::Read(NxsToken &token)
{ 
  DemandEndSemicolon(token, "runconfig");

  while(true)
    {
      token.GetNextToken();
      NxsBlock::NxsCommandResult res = HandleBasicBlockCommands(token); 

      if (res == NxsBlock::NxsCommandResult(STOP_PARSING_BLOCK))
	return;
      if (res != NxsBlock::NxsCommandResult(HANDLED_COMMAND))
	{
	  auto key = token.GetToken(false);
	  token.GetNextToken(); 
	  auto value = token.GetToken(false); 	    

	  if(key.EqualsCaseInsensitive("numGen"))
	    numGen = value.ConvertToInt(); 
	  else if (key.EqualsCaseInsensitive("parsimonyStartingTree"))
	    useParsimonyStarting = convertToBool(value); 
	  else if (key.EqualsCaseInsensitive("checkpointinterval"))
	    chkpntFreq = value.ConvertToInt(); 
	  else if(key.EqualsCaseInsensitive("samplingfrequency"))
	    samplingFreq = value.ConvertToInt(); 
	  else if(key.EqualsCaseInsensitive("componentWiseMH"))
	    componentWiseMH = convertToBool(value); 
	  else if(key.EqualsCaseInsensitive("numswaps"))
	    numSwaps = value.ConvertToInt(); 
	  else if(key.EqualsCaseInsensitive("numRuns"))	    
	    numRunConv = value.ConvertToInt(); 
	  else if(key.EqualsCaseInsensitive("diagFreq"))
	    diagFreq = value.ConvertToInt(); 
	  else if(key.EqualsCaseInsensitive("heatedChainsUseSame"))
	    heatedChainsUseSame = convertToBool(value); 
	  else if(key.EqualsCaseInsensitive("numcoupledChains"))
	    numCoupledChains = value.ConvertToInt(); 
	  else if(key.EqualsCaseInsensitive("printFreq") )	   
	    printFreq = value.ConvertToInt();
	  else if (key.EqualsCaseInsensitive("asdsfIgnoreFreq"))
	    asdsfIgnoreFreq = value.ConvertToDouble(); 
	  else if (key.EqualsCaseInsensitive("asdsfConvergence"))
	    asdsfConvergence = value.ConvertToDouble();
	  else if (key.EqualsCaseInsensitive("heatFactor"))
	    heatFactor = value.ConvertToDouble();
	  else if(key.EqualsCaseInsensitive("swapInterval"))
	    swapInterval = value.ConvertToInt();
	  else if(key.EqualsCaseInsensitive("tuneHeat"))
	    tuneHeat = convertToBool(value);
	  else if(key.EqualsCaseInsensitive("tuneFreq"))
	    tuneFreq = value.ConvertToInt();
	  else if(key.EqualsCaseInsensitive("burninGen"))
	    burninGen = value.ConvertToInt();
	  else if(key.EqualsCaseInsensitive("burninProportion"))
	    burninProportion = value.ConvertToDouble();
	  else if(key.EqualsCaseInsensitive("asdsfusemax"))
	    useAsdsfMax = convertToBool(value);
	  else 	      
	    cerr << "WARNING: ignoring unknown value >"  << key << "< and >" << value <<  "<" << endl; 
	}
    }
}


static void verifyProbability(double value, bool lowerIncluded, bool upperIncluded, std::string name)
{
  auto lowOkay = 0 < value || ( lowerIncluded &&  0 ==  value) ; 
  auto upperOkay = value < 1. || (upperIncluded && value == 1); 

  char lowBracket = lowerIncluded ? '[': '('; 
  char upperBracket = upperIncluded ? ']' : ')' ; 

  if( not ( lowOkay && upperOkay ) )
    {
      std::cerr << "Error: >" << name << "< must be in the interval " << lowBracket  << "0,1" << upperBracket << std::endl; 
      exit(-1); 
    }

}

static void verifyGreaterZero(int value, std::string name )
{
  if( not ( value > 0 )   )
    {
      std::cout << "Error: >name< must be > 0 "  << std::endl; 
      exit(-1); 
    }  
}




void BlockRunParameters::verify() const 
{
  verifyGreaterZero(diagFreq, "diagFreq"); 
  
  verifyProbability(asdsfIgnoreFreq, true, true , "asdsfIgnoreFreq" ); 
  verifyProbability(asdsfConvergence, false, false, "asdsfConvergence"); 
  verifyProbability(burninProportion, false, false, "burninProportion"); 
  verifyProbability(heatFactor, false, false, "heatFactor"); 

  verifyGreaterZero(numSwaps, "numSwaps"); 
  verifyGreaterZero(samplingFreq,"samplingFreq" ); 
  verifyGreaterZero(numRunConv, "numRunConv"); 
  verifyGreaterZero(numGen, "numGen"); 
  verifyGreaterZero(numCoupledChains, "numCoupledChains"); 
  verifyGreaterZero(printFreq, "printFreq"); 
  verifyGreaterZero(swapInterval, "swapInterval"); 
  verifyGreaterZero(tuneFreq, "tuneFreq");  

  if( diagFreq <= nat(samplingFreq)  ) 
    {
      std::cerr << "diagFreq < samplingFreq. Please choose the sampling frequency smaller than the diagnosis frequency.  " << std::endl; 
      exit(-1);
    }
}
