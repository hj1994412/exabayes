#ifndef OUTPUT_FILE_H
#define OUTPUT_FILE_H

#include <sys/stat.h>
#include <sstream>

///////////////////////////////////////////////////////////////////////////////
//                                OUTPUT FILE                                //
///////////////////////////////////////////////////////////////////////////////
class OutputFile
{
    ///////////////////////////////////////////////////////////////////////////
    //                            PUBLIC INTERFACE                           //
    ///////////////////////////////////////////////////////////////////////////
public:
    // ________________________________________________________________________
    OutputFile()
        : fullFileName{""}
    {}
    // ________________________________________________________________________
    virtual ~OutputFile(){}
    // ________________________________________________________________________
    static void                           rejectIfExists(
        std::string fileName);
    // ________________________________________________________________________
    void                                  rejectIfNonExistant(
        std::string fileName);
    // TODO portability
    // ________________________________________________________________________
    static std::string                    getFileBaseName(
        std::string workdir);
    // ________________________________________________________________________
    std::string                           getFileName() const
    {return fullFileName; }
    // ________________________________________________________________________
    static bool                           directoryExists(
        std::string name);
    // ________________________________________________________________________
    void                                  removeMe() const;

    ///////////////////////////////////////////////////////////////////////////
    //                             PROTECTED DATA                            //
    ///////////////////////////////////////////////////////////////////////////
protected:
    std::string fullFileName;
};


#endif
