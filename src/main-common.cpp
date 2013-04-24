#include "axml.h"
#include "main-common.h"
#include "bayes.h"
#include "globals.h" 


// void printVersionInfo()
// {
// #if HAVE_PLL != 0
//   PRINT("\n\nThis is %s, version %s built with the phlogenetic likelihood library.\n", PROGRAM_NAME, VERSION); 
// #else 
//   PRINT("\n\nThis is %s, version %s\n", PROGRAM_NAME, VERSION); 
// #endif  
// }

// void printREADME()
// {
//   printf("TODO\n"); 
// }


// void finalizeFiles()
// {
//   PRINT("TODO\n"); 
// }




// void parseCommandLine(int argc, char *argv[], analdef *adef)
// {
  
// }




/* TODO consolidate */
// void get_args_old(int argc, char *argv[], analdef *adef, tree *tr)
// {
//   assert(0); 
  
//   boolean
//     bad_opt    = FALSE,
//     resultDirSet = FALSE;

//   char
//     resultDir[1024] = "",          
//     *optarg,
//     model[1024] = ""
//   ;

//   double 
//     likelihoodEpsilon;
  
//   int     
//     optind = 1,        
//     c,
//     nameSet = 0,
//     treeSet = 0,   
//     modelSet = 0, 
//     byteFileSet = 0;


//   /*********** tr inits **************/ 
 
//   tr->doCutoff = TRUE;
//   tr->secondaryStructureModel = SEC_16; /* default setting */
//   tr->searchConvergenceCriterion = FALSE;
//   tr->rateHetModel = GAMMA;
 
//   tr->multiStateModel  = GTR_MULTI_STATE;
// #if HAVE_PLL == 0 
//     tr->useGappedImplementation = FALSE;
//     tr->saveBestTrees          = 0;
// #endif
//   tr->saveMemory = FALSE;

//   tr->manyPartitions = FALSE;

//   tr->categories             = 25;

//   tr->grouped = FALSE;
//   tr->constrained = FALSE;

//   tr->gapyness               = 0.0; 


//   tr->useMedian = FALSE;

//   int seed = -1 ;

//   strcpy(configFileName, "\0");

//   /* legacy  */
//   strcpy(model, "GAMMA"); 
//   modelSet = 1; 

//   /********* tr inits end*************/




//   while(!bad_opt && ((c = mygetopt(argc,argv,"T:e:c:f:i:t:w:n:s:vhMQap:", &optind, &optarg))!=-1))
//     {
//     switch(c)
//       {    
//       // case 'a':
//       // 	tr->useMedian = TRUE;
//       // 	break;
//       case 'Q':			// TODO  
// 	tr->manyPartitions = TRUE;   	
// 	break;
//       case 's': 
// 	strcpy(byteFileName, optarg);	 	
// 	byteFileSet = TRUE;
// 	break;      
//       case 'M':
// 	adef->perGeneBranchLengths = TRUE;
// 	break;                                 
//       case 'e':
// 	sscanf(optarg,"%lf", &likelihoodEpsilon);
// 	adef->likelihoodEpsilon = likelihoodEpsilon;
// 	break;          
//       case 'v':
// 	printVersionInfo();
// 	errorExit(0);
      
//       case 'h':
// 	printREADME();
// 	errorExit(0); 
//       case 'i':
// 	sscanf(optarg, "%d", &adef->initial);
// 	adef->initialSet = TRUE;
// 	break;
//       case 'n':
//         strcpy(run_id,optarg);
// 	analyzeRunId(run_id);
// 	nameSet = 1;
//         break;
//       case 'w':
//         strcpy(resultDir, optarg);
// 	resultDirSet = TRUE;
//         break;
//       case 'T':
// #ifdef _USE_PTHREADS
// 	sscanf(optarg,"%d", &(tr->numberOfThreads));
// #else
// 	printf("Option -T does not have any effect with the sequential or parallel MPI version.\n");
// 	printf("It is used to specify the number of threads for the Pthreads-based parallelization\n");
// #endif
// 	break;
//       case 't':
// 	strcpy(tree_file, optarg); 
// 	treeSet = 1;       
// 	break;     
//       case 'c': 
// 	strcpy(configFileName, optarg); 
// 	break; 
//       case 'p': 
// 	{
// 	  seed = atoi(optarg);
// 	  gAInfo.rGlobalKey.v[0] = seed; 
// 	  gAInfo.rGlobalKey.v[1] = 0xbadcafe; /* how funny */
// 	  break; 
// 	}
//       default:
// 	errorExit(-1);
//       }
//     }

  
//   if(strlen(configFileName) == 0 ||  ! filexists(configFileName) )
//     {
//       PRINT("\nPlease provide a config file via -C <file>. A testing file is available under  \n"); 
//       errorExit(-1); 
//     }

  
//   if(seed == -1 )
//     {
//       PRINT("\nPlease provide a proper seed for the initialization of the random number generator. \n "); 
//       errorExit(-1); 
//     }

//   if( ! filexists( configFileName))
//     {

//       PRINT("\nPlease provide a minimal config file via -C <file>.\n");
//       errorExit(-1);
//     }


//   if(!byteFileSet)
//     {
//       PRINT("\nError, you must specify a binary format data file with the \"-s\" option\n");
//       errorExit(-1);
//     }

//   if(!modelSet)
//     {
//       PRINT("\nError, you must specify a model of rate heterogeneity with the \"-m\" option\n");
//       errorExit(-1);
//     }

//   if(!nameSet)
//     {
//       PRINT("\nError: please specify a name for this run with -n\n");
//       errorExit(-1);
//     }


//   if( ! treeSet)
//     {
//       printf("no tree file provided, will use random trees as initial state.\n "); 
//       char tmp[1024]; 
//       tmp[0] = '\0'; 
//       strcpy(tree_file, tmp); 
//       gAInfo.numberOfStartingTrees = 0; 
//       /* tree_file = "\0";  */
//     }
//   else 
//     {
//       FILE *fh = myfopen(tree_file, "r"); 
//       gAInfo.numberOfStartingTrees = 0;  
//       int ch; 

//       while((ch = fgetc(fh)) != EOF)
// 	if(ch == ';')
// 	  gAInfo.numberOfStartingTrees++;
      
//       printf("%d starting trees provided via -t\n", gAInfo.numberOfStartingTrees); 
//     }

//    {

//     const 
//       char *separator = "/";

//     if(resultDirSet)
//       {
// 	char 
// 	  dir[1024] = "";
	

// 	if(resultDir[0] != separator[0])
// 	  strcat(dir, separator);
	
// 	strcat(dir, resultDir);
	
// 	if(dir[strlen(dir) - 1] != separator[0]) 
// 	  strcat(dir, separator);
// 	strcpy(workdir, dir);
//       }
//     else
//       {
// 	char 
// 	  dir[1024] = "",
// 	  *result = getcwd(dir, sizeof(dir));
	
// 	assert(result != (char*)NULL);
	
// 	if(dir[strlen(dir) - 1] != separator[0]) 
// 	  strcat(dir, separator);
	
// 	strcpy(workdir, dir);		
//       }
//    }

//   return;
// }

