#ifndef _MAIN_COMMON_H 
#define _MAIN_COMMON_H 


/* provides  */
void printVersionInfo(); 
void printREADME(); 
int mygetopt(int argc, char **argv, char *opts, int *optind, char **optarg); 
void get_args(int argc, char *argv[], analdef *adef, tree *tr); 
void analyzeRunId(char id[128]); 
void initAdef(analdef *adef); 
void ignoreExceptionsDenormFloat(); 
int filexists(char *filename); 

#endif
