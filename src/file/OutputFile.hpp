#ifndef OUTPUT_FILE_H 
#define OUTPUT_FILE_H 

#include <sys/stat.h>
#include <sstream>

#include "ParallelSetup.hpp"

class OutputFile
{
public: 
  void rejectIfExists(std::string fileName)
  {
    if( std::ifstream(fileName) ) 
      {
	std::cerr << std::endl <<  "File " << fileName << " already exists (probably \n"
		  << "from previous run). Please choose a new run-id or remove previous output files. " << std::endl; 
	ParallelSetup::genericExit(-1);
    }
  }

  
  void rejectIfNonExistant(std::string fileName)
  {
    if( not std::ifstream(fileName) )      
      {
	std::cerr << "Error: could not find file from previous run. \n"
		  << "The assumed name of this file was >" <<  fileName << "<. Aborting." << std::endl; 
	ParallelSetup::genericExit(0); 
      }
  }

// TODO portability 
  
  static std::string getFileBaseName(std::string workdir )
  {
    std::stringstream ss; 
    ss << workdir << ( workdir.compare("") == 0  ? "" : "/") << PROGRAM_NAME; 
    return ss.str(); 
  }


  static bool directoryExists(std::string name)
  {
    struct stat st;
    if(stat(name.c_str(),&st) == 0)
      if((st.st_mode & S_IFDIR) != 0)
	return true; 
    return false; 
  }



protected:   
  std::string fullFileName;   
}; 

#endif