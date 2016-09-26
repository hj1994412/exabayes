/**
 *  @file exabayes.cpp
 *
 *  @brief This file sets the flavour of exabayes that has been
 *  compiled (i.e., sequential, pll, examl, ...)
 *
 */


// TODO re-activate all that initial bla bla when starting up
// exa-bayes (model / program info )


// dirty...
// #include  <google/heap-profiler.h>

#ifdef HAVE_AVX
#define __AVX
#endif

#include "releaseDate.hpp"

#include "CommandLine.hpp"
#include "SampleMaster.hpp"
#include "ParallelSetup.hpp"

#include "TeeStream.hpp"

#include <iostream>
#include <sstream>
#include <chrono>
#include <sstream>

using std::stringstream;
using std::cout;
using std::endl;

void                           exa_main(
    CommandLine&   cl,
    ParallelSetup* pl);

/*
 * tell the CPU to ignore exceptions generated by denormalized floating point
 * values.
 * If this is not done, depending on the input data, the likelihood functions
 * can exhibit
 * substantial run-time differences for vectors of equal length.
 */

static void                    ignoreExceptionsDenormFloat()
{
#if !(defined(__ppc) || defined(__powerpc__) || defined(PPC))
    _mm_setcsr(_mm_getcsr() | _MM_FLUSH_ZERO_ON);
#endif
}


static bool                    fileExists(
    const std::string&name)
{
    auto&&ifh = std::ifstream{
        name
    };
    bool  result = ifh.is_open();
    ifh.close();
    return result;
}


static void                    initLogFile(
    const ParallelSetup&pl,
    std::string         logFile)
{
    if (pl.isGlobalMaster())
        logStream =  make_unique<std::ofstream>(logFile);
    else
        logStream = make_unique<std::ofstream>(std::string{"/dev/null"});

    teeOut =  make_unique<TeeStream>(std::cout, *logStream, MY_TID);

    if (not pl.isGlobalMaster())
        teeOut->disable();
}


static void                    initializeProfiler(
    ParallelSetup& pl)
{
    // see this page for info
    // http://google-perftools.googlecode.com/svn/trunk/doc/cpuprofile.html
    // that option is important
    // CPUPROFILE_FREQUENCY=x
#ifdef _USE_GOOGLE_PROFILER
    auto&& ss = std::stringstream{};
    ss << "profile.out." << pl.getGlobalComm().getRank();
    auto   myProfileFile = ss.str();
    remove(myProfileFile.c_str());
    ProfilerStart(myProfileFile.c_str());
#endif

    // HeapProfilerStart("heap.profile");
}


static void                    finalizeProfiler()
{
#ifdef _USE_GOOGLE_PROFILER
    ProfilerStop();
#endif
    // HeapProfilerStop();
}


static void                    printInfoHeader(
    CommandLine& cl)
{
    auto&&ss = std::stringstream{};

    ss << "\n";

    ss << "This is ";

    if (isYggdrasil)
        ss << "Yggdrasil, the multi-threaded variant of ";
    else
        ss << "the multi-threaded MPI hybrid variant of ";

    ss << PROGRAM_NAME << " (version " << VERSION << "),\n";
    ss
        <<
        "a tool for Bayesian MCMC sampling of phylogenetic trees, build with "
        "the";

    ss << "\nPhylogenetic Likelihood Library (version "
       << PLL_LIB_VERSION << ", " << PLL_LIB_DATE << ").";

    ss <<  "\n\nThis software has been released on " << RELEASE_DATE
       << "\n(git commit id:" << GIT_COMMIT_ID << ")"
                                               "\n\n\tby Andre J. Aberer, Kassian Kobert and Alexandros "
                                               "Stamatakis\n"
       << "\nPlease send any bug reports, feature requests and inquiries to "
       << PACKAGE_BUGREPORT << "\n\n";

    ss << "The program was called as follows: \n";
    ss << cl.getCommandLineString() << "\n";
    ss
        <<
        "\n================================================================\n";

    tout  << ss.str();
}


// also the entry point for the threads

/**
 * @brief the main ExaBayes function.
 *
 * @param tr -- a tree structure that has been initialize in one of the adapter
 * mains.
 * @param adef -- the legacy adef
 */
void                           exa_main(
    CommandLine&   cl,
    ParallelSetup* pl)
{
    if (cl.hasThreadPinning())
        pl->pinThreads();

    // auto && ss = std::stringstream();
    // ss << *pl << "\n\n";

    // ss <<  *pl << "\n\n";
    // std::cout << SyncOut() << ss.str();

    ignoreExceptionsDenormFloat();

    printInfoHeader(cl);

    // timeIncrement = CLOCK::system_clock::now();

    auto&& master = SampleMaster(pl->getGlobalComm().size());

    master.setParallelSetup(pl);
    master.setCommandLine(cl);

    master.initializeRuns(Randomness(cl.getSeed()));

    if (cl.isDryRun())
    {
        std::cout
            <<
            "Command line, input data and config file is okay. Exiting gracefully."
            << std::endl;
        exitFunction(0, true);
    }

    master.simulate();
    master.finalizeRuns();
}


static void                    makeInfoFile(
    const CommandLine&  cl,
    const ParallelSetup&pl)
{
    auto&&ss = stringstream{};

    if (cl.isDryRun())
        ss << "/dev/null";
    else
        ss << OutputFile::getFileBaseName(cl.getWorkdir()) << "_info."
           << cl.getRunid();

    if (not cl.isDryRun() && pl.isGlobalMaster())
    {
        if (fileExists(ss.str()))
        {
            std::cerr << std::endl <<  "File " << ss.str()
                      << " already exists (probably \n"
                      <<
                "from previous run). Please choose a new run-id or remove previous output files. "
                      << std::endl;
            exitFunction(-1, true);
        }
    }

    initLogFile(pl, ss.str());
}


// just having this, because of mpi_finalize
static int                     innerMain(
    int   argc,
    char**argv)
{
    _masterThread = std::this_thread::get_id();
    _threadsDie = false;

    auto cl = CommandLine();
    cl.initialize(argc, argv);

    auto plPtr = make_unique<ParallelSetup>(cl);
    plPtr->initialize();

    if (cl.onlyPrintHelp())
    {
        auto&&ss = stringstream{};

        if (plPtr->isGlobalMaster())
            cl.printHelp(ss);

        cout << SyncOut()  << ss.str() << endl;
        exitFunction(0, true);
    }
    else if (cl.onlyPrintVersion())
    {
        auto&&ss = stringstream{};

        if (plPtr->isGlobalMaster())
            cl.printVersion(ss);

        cout << SyncOut() << ss.str() << endl;
        exitFunction(0, true);
    }


    makeInfoFile(cl, *plPtr);

    plPtr->releaseThreads();

    // notice: per-thread profiling does not work...
    initializeProfiler(*plPtr);
    exa_main(cl, plPtr.get());
    finalizeProfiler();

    return 0;
}


static void                    myExit(
    int  code,
    bool waitForAll)
{
    ParallelSetup::abort(code, waitForAll);
}


int                            main(
    int   argc,
    char**argv)
{
#ifdef _IS_YGG
    isYggdrasil  = true;
    exitFunction = myExit;
#else
    isYggdrasil  = false;
    exitFunction = myExit;
#endif
    ParallelSetup::initializeRemoteComm(argc, argv);

#ifndef HAVE_LONG_LONG
    std::cout << "Danger: could not find 64-bit integers. " << PROGRAM_NAME
              << " will probably not work correctly." << std::endl;
#endif


    innerMain(argc, argv);
    ParallelSetup::finalize();
    return 0;
}
