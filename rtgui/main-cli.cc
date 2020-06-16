/*
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef __GNUC__
#if defined(__FAST_MATH__)
#error Using the -ffast-math CFLAG is known to lead to problems. Disable it to compile ART.
#endif
#endif

#include "config.h"
#include <gtkmm.h>
#include <giomm.h>
#include <iostream>
#include <tiffio.h>
#include <cstring>
#include <cstdlib>
#include <locale.h>
#include "options.h"
#include "soundman.h"
#include "rtimage.h"
#include "version.h"
#include "extprog.h"
#include "printhelp.h"
#include "pathutils.h"
#include "../rtengine/profilestore.h"
#include "fastexport.h"

#ifndef WIN32
#include <glibmm/fileutils.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glibmm/threads.h>
#else
#include <glibmm/thread.h>
#include <windows.h>
#include "conio.h"
#endif

#include <thread>
#include <chrono>

// Set this to 1 to make RT work when started with Eclipse and arguments, at least on Windows platform
#define ECLIPSE_ARGS 0

extern Options options;

// stores path to data files
Glib::ustring argv0;
Glib::ustring creditsPath;
Glib::ustring licensePath;
Glib::ustring argv1;
bool progress = false;
//bool simpleEditor;
//Glib::Threads::Thread* mainThread;

namespace
{

// For an unknown reason, Glib::filename_to_utf8 doesn't work on reliably Windows,
// so we're using Glib::filename_to_utf8 for Linux/Apple and Glib::locale_to_utf8 for Windows.
Glib::ustring fname_to_utf8 (const char* fname)
{
#ifdef WIN32

    try {
        return Glib::locale_to_utf8 (fname);
    } catch (Glib::Error&) {
        return Glib::convert_with_fallback (fname, "UTF-8", "ISO-8859-1", "?");
    }

#else

    return Glib::filename_to_utf8 (fname);

#endif
}

bool fast_export = false;

typedef std::unique_ptr<rtengine::procparams::PartialProfile> PartialProfile;

bool check_partial_profile(const PartialProfile &pp)
{
    rtengine::procparams::ProcParams params;
    return pp->applyTo(params);
}

} // namespace


/* Process line command options
 * Returns
 *  0 if process in batch has executed
 *  1 to start GUI (with a dir or file option)
 *  2 to start GUI because no files found
 *  -1 if there is an error in parameters
 *  -2 if an error occurred during processing
 *  -3 if at least one required procparam file was not found */
int processLineParams ( int argc, char **argv );

std::pair<bool, bool> dontLoadCache(int argc, char **argv);

int main (int argc, char **argv)
{
    setlocale (LC_ALL, "");
    setlocale (LC_NUMERIC, "C"); // to set decimal point to "."

    Gio::init ();

    //mainThread = Glib::Threads::Thread::self();

#ifdef BUILD_BUNDLE
    char exname[512] = {0};
    Glib::ustring exePath;
    // get the path where the rawtherapee executable is stored
#ifdef WIN32
    WCHAR exnameU[512] = {0};
    GetModuleFileNameW (NULL, exnameU, 511);
    WideCharToMultiByte (CP_UTF8, 0, exnameU, -1, exname, 511, 0, 0 );
#else

    if (readlink ("/proc/self/exe", exname, 511) < 0) {
        strncpy (exname, argv[0], 511);
    }

#endif
    exePath = Glib::path_get_dirname (exname);

    // set paths
    if (Glib::path_is_absolute (DATA_SEARCH_PATH)) {
        argv0 = DATA_SEARCH_PATH;
    } else if (DATA_SEARCH_PATH == ".") {
        argv0 = exePath;
    } else {
        argv0 = Glib::build_filename (exePath, DATA_SEARCH_PATH);
    }

    if (Glib::path_is_absolute (CREDITS_SEARCH_PATH)) {
        creditsPath = CREDITS_SEARCH_PATH;
    } else {
        creditsPath = Glib::build_filename (exePath, CREDITS_SEARCH_PATH);
    }

    if (Glib::path_is_absolute (LICENCE_SEARCH_PATH)) {
        licensePath = LICENCE_SEARCH_PATH;
    } else {
        licensePath = Glib::build_filename (exePath, LICENCE_SEARCH_PATH);
    }

    options.rtSettings.lensfunDbDirectory = LENSFUN_DB_PATH;

#else
    argv0 = DATA_SEARCH_PATH;
    creditsPath = CREDITS_SEARCH_PATH;
    licensePath = LICENCE_SEARCH_PATH;
    options.rtSettings.lensfunDbDirectory = LENSFUN_DB_PATH;
#endif

    auto p = dontLoadCache(argc, argv);
    bool quickstart = p.first, verbose = p.second;

    try {
        Options::load(quickstart, verbose);
    } catch (Options::Error &e) {
        std::cerr << std::endl
                  << "Error:" << e.get_msg() << std::endl;
        return -2;
    }

    if (options.is_defProfRawMissing()) {
        options.defProfRaw = DEFPROFILE_RAW;
        std::cerr << std::endl
                  << "The default profile for raw photos could not be found or is not set." << std::endl
                  << "Please check your profiles' directory, it may be missing or damaged." << std::endl
                  << "\"" << DEFPROFILE_RAW << "\" will be used instead." << std::endl << std::endl;
    }
    if (options.is_bundledDefProfRawMissing()) {
        std::cerr << std::endl
                  << "The bundled profile \"" << options.defProfRaw << "\" could not be found!" << std::endl
                  << "Your installation could be damaged." << std::endl
                  << "Default internal values will be used instead." << std::endl << std::endl;
        options.defProfRaw = DEFPROFILE_INTERNAL;
    }

    if (options.is_defProfImgMissing()) {
        options.defProfImg = DEFPROFILE_IMG;
        std::cerr << std::endl
                  << "The default profile for non-raw photos could not be found or is not set." << std::endl
                  << "Please check your profiles' directory, it may be missing or damaged." << std::endl
                  << "\"" << DEFPROFILE_IMG << "\" will be used instead." << std::endl << std::endl;
    }
    if (options.is_bundledDefProfImgMissing()) {
        std::cerr << std::endl
                  << "The bundled profile " << options.defProfImg << " could not be found!" << std::endl
                  << "Your installation could be damaged." << std::endl
                  << "Default internal values will be used instead." << std::endl << std::endl;
        options.defProfImg = DEFPROFILE_INTERNAL;
    }

    TIFFSetWarningHandler (nullptr);   // avoid annoying message boxes

#ifndef WIN32

    // Move the old path to the new one if the new does not exist
    if (Glib::file_test (Glib::build_filename (options.rtdir, "cache"), Glib::FILE_TEST_IS_DIR) && !Glib::file_test (options.cacheBaseDir, Glib::FILE_TEST_IS_DIR)) {
        if (g_rename (Glib::build_filename (options.rtdir, "cache").c_str (), options.cacheBaseDir.c_str ()) == -1) {
            std::cout << "g_rename " <<  Glib::build_filename (options.rtdir, "cache").c_str () << " => " << options.cacheBaseDir.c_str () << " failed." << std::endl;
        }
    }

#endif

    int ret = 0;

    // printing RT's version in all case, particularly useful for the 'verbose' mode, but also for the batch processing
    std::cout << RTNAME << ", version " << RTVERSION << ", command line." << std::endl;

    if (argc > 1) {
        ret = processLineParams (argc, argv);
    } else {
        std::cout << "Terminating without anything to do." << std::endl;
    }

    return ret;
}


std::pair<bool, bool> dontLoadCache(int argc, char **argv)
{
    bool quick = false;
    bool verbose = false;
    
    for (int iArg = 1; iArg < argc; iArg++) {
        Glib::ustring currParam (argv[iArg]);
#if ECLIPSE_ARGS
        currParam = currParam.substr (1, currParam.length() - 2);
#endif
        if (currParam.length() > 1 && currParam[0] == '-') {
            switch (currParam[1]) {
            case 'q':
                quick = true;
                break;
            case 'v':
                std::cout << RTNAME << ", version " << RTVERSION << ", command line." << std::endl;
                exit(0);
            case '?':
            case 'h':
                ART_print_help(argv[0], false);
                exit(0);
            case 'V':
                verbose = true;
                break;
            case '-':
                if (currParam == "--progress") {
                    progress = true;
                }
                break;
            default:
                break;
            }
        }
    }

    if (progress) {
        verbose = false;
    }

    return std::make_pair(quick, verbose);
}


class ConsoleProgressListener: public rtengine::ProgressListener {
public:
    ConsoleProgressListener(int num_steps):
        num_steps_(num_steps), percent_(0) {}

    void incr()
    {
        MyMutex::MyLock l(mutex_);
        percent_ += 100.f / num_steps_;
    }
    
    void setProgress(double p)
    {
        MyMutex::MyLock l(mutex_);
        int pct = (p * 100) / num_steps_;
        std::cout << "\n" << rtengine::LIM(percent_ + pct, 0, 99) << std::endl;
    }
    
    void setProgressStr(const Glib::ustring &str) {}
    void setProgressState(bool inProcessing) {}

    void error(const Glib::ustring &msg)
    {
        MyMutex::MyLock l(mutex_);
        std::cerr << "Error: " << msg << std::endl;
    }

    void info(const Glib::ustring &msg)
    {
        MyMutex::MyLock l(mutex_);
        std::cout << "  " << msg << std::endl;
    }        

    void msg(const Glib::ustring &msg)
    {
        MyMutex::MyLock l(mutex_);
        std::cout << "\n#" << msg << std::endl;
    }

    void ping()
    {
        MyMutex::MyLock l(mutex_);
        std::cout.put(' ');
        std::cout.flush();
        if (!std::cout) {
            _exit(0);
        }
    }
    
private:
    MyMutex mutex_;
    int num_steps_;
    int percent_;
};


int processLineParams ( int argc, char **argv )
{
    PartialProfile rawParams, imgParams;
    std::vector<Glib::ustring> inputFiles;
    Glib::ustring outputPath = "";
    std::vector<PartialProfile> processingParams;
    bool outputDirectory = false;
    bool leaveUntouched = false;
    bool overwriteFiles = false;
    bool sideProcParams = false;
    bool copyParamsFile = false;
    bool skipIfNoSidecar = false;
    bool allExtensions = false;
    bool useDefault = false;
    unsigned int sideCarFilePos = 0;
    int compression = 92;
    int subsampling = 3;
    int bits = -1;
    bool isFloat = false;
    std::string outputType = "";
    unsigned errors = 0;

    for ( int iArg = 1; iArg < argc; iArg++) {
        Glib::ustring currParam (argv[iArg]);
        if ( currParam.empty() ) {
            continue;
        }
#if ECLIPSE_ARGS
        currParam = currParam.substr (1, currParam.length() - 2);
#endif

        if ( currParam.at (0) == '-' && currParam.size() > 1) {
            switch ( currParam.at (1) ) {
                case '-':
                    // GTK --argument, we're skipping it
                    break;

                case 'O':
                    copyParamsFile = true;

                case 'o': // outputfile or dir
                    if ( iArg + 1 < argc ) {
                        iArg++;
                        outputPath = Glib::ustring (fname_to_utf8 (argv[iArg]));
#if ECLIPSE_ARGS
                        outputPath = outputPath.substr (1, outputPath.length() - 2);
#endif

                        if (outputPath.substr (0, 9) == "/dev/null") {
                            outputPath.assign ("/dev/null"); // removing any useless chars or filename
                            outputDirectory = false;
                            leaveUntouched = true;
                        } else if (Glib::file_test (outputPath, Glib::FILE_TEST_IS_DIR)) {
                            outputDirectory = true;
                        }
                    }

                    break;

                case 'p': // processing parameters for all inputs; all set procparams are required, so

                    // RT stop if any of them can't be loaded for any reason.
                    if ( iArg + 1 < argc ) {
                        iArg++;
                        Glib::ustring fname (fname_to_utf8 (argv[iArg]));
#if ECLIPSE_ARGS
                        fname = fname.substr (1, fname.length() - 2);
#endif

                        if (fname.at (0) == '-') {
                            std::cerr << "Error: filename missing next to the -p switch." << std::endl;
                            return -3;
                        }

                        PartialProfile currentParams(new rtengine::procparams::FilePartialProfile(nullptr, fname));

                        if (check_partial_profile(currentParams)) {
                            processingParams.push_back(std::move(currentParams));
                        } else {
                            std::cerr << "Error: \"" << fname << "\" not found." << std::endl;
                            return -3;
                        }
                    }

                    break;

                case 'S':
                    skipIfNoSidecar = true;

                case 's': // Processing params next to file (file extension appended)
                    sideProcParams = true;
                    sideCarFilePos = processingParams.size();
                    break;

                case 'd':
                    useDefault = true;
                    break;

                case 'q':
                    break;

                case 'Y':
                    overwriteFiles = true;
                    break;

                case 'a':
                    allExtensions = true;
                    break;

                case 'j':
                    if (currParam.length() > 2 && currParam.at (2) == 's') {
                        if (currParam.length() == 3) {
                            std::cerr << "Error: the -js switch requires a mandatory value!" << std::endl;
                            return -3;
                        }

                        // looking for the subsampling parameter
                        subsampling = atoi (currParam.substr (3).c_str());

                        if (subsampling < 1 || subsampling > 3) {
                            std::cerr << "Error: the value accompanying the -js switch has to be in the [1-3] range!" << std::endl;
                            return -3;
                        }
                    } else {
                        outputType = "jpg";
                        if(currParam.size() < 3) {
                            compression = 92;
                        } else {
                            compression = atoi (currParam.substr (2).c_str());

                            if (compression < 0 || compression > 100) {
                                std::cerr << "Error: the value accompanying the -j switch has to be in the [0-100] range!" << std::endl;
                                return -3;
                            }
                        }
                    }

                    break;

                case 'b':
                    bits = atoi (currParam.substr (2).c_str());

                    if (currParam.length() >= 3 && currParam.at(2) == '8') { // -b8
                        bits = 8;
                    } else if (currParam.length() >= 4 && currParam.length() <= 5 && currParam.at(2) == '1' && currParam.at(3) == '6') { // -b16, -b16f
                        bits = 16;
                        if (currParam.length() == 5 && currParam.at(4) == 'f') {
                            isFloat = true;
                        }
                    } else if (currParam.length() >= 4 && currParam.length() <= 5 && currParam.at(2) == '3' && currParam.at(3) == '2') { // -b32 == -b32f
                        bits = 32;
                        isFloat = true;
                    }

                    if (bits != 8 && bits != 16 && bits != 32) {
                        std::cerr << "Error: specify output bit depth per channel as -b8 for 8-bit integer, -b16 for 16-bit integer, -b16f for 16-bit float or -b32 for 32-bit float." << std::endl;
                        return -3;
                    }

                    break;

                case 't':
                    outputType = "tif";
                    compression = ((currParam.size() < 3 || currParam.at (2) != 'z') ? 0 : 1);
                    break;

                case 'n':
                    outputType = "png";
                    compression = -1;
                    break;

                case 'f':
                    fast_export = true;
                    break;

                case 'c': // MUST be last option
                    while (iArg + 1 < argc) {
                        iArg++;
                        Glib::ustring argument (fname_to_utf8 (argv[iArg]));
#if ECLIPSE_ARGS
                        argument = argument.substr (1, argument.length() - 2);
#endif

                        if (!Glib::file_test (argument, Glib::FILE_TEST_EXISTS)) {
                            std::cerr << "\"" << argument << "\"  doesn't exist!" << std::endl;
                            continue;
                        }

                        if (Glib::file_test (argument, Glib::FILE_TEST_IS_REGULAR)) {
                            bool notAll = allExtensions && !options.is_parse_extention (argument);
                            bool notRetained = !allExtensions && !options.has_retained_extention (argument);

                            if (notAll || notRetained) {
                                if (notAll) {
                                    std::cout << "\"" << argument << "\"  is not one of the parsed extensions. Image skipped." << std::endl;
                                } else if (notRetained) {
                                    std::cout << "\"" << argument << "\"  is not one of the selected parsed extensions. Image skipped." << std::endl;
                                }
                            } else {
                                inputFiles.emplace_back (argument);
                            }

                            continue;

                        }

                        if (Glib::file_test (argument, Glib::FILE_TEST_IS_DIR)) {

                            auto dir = Gio::File::create_for_path (argument);

                            if (!dir || !dir->query_exists()) {
                                continue;
                            }

                            try {

                                auto enumerator = dir->enumerate_children ("standard::name,standard::type");

                                while (auto file = enumerator->next_file()) {

                                    const auto fileName = Glib::build_filename (argument, file->get_name());
                                    bool isDir = file->get_file_type() == Gio::FILE_TYPE_DIRECTORY;
                                    bool notAll = allExtensions && !options.is_parse_extention (fileName);
                                    bool notRetained = !allExtensions && !options.has_retained_extention (fileName);

                                    if (isDir || notAll || notRetained) {
                                        if (isDir) {
                                            std::cout << "\"" << fileName << "\"  is a folder. Folder skipped" << std::endl;
                                        } else if (notAll) {
                                            std::cout << "\"" << fileName << "\"  is not one of the parsed extensions. Image skipped." << std::endl;
                                        } else if (notRetained) {
                                            std::cout << "\"" << fileName << "\"  is not one of the selected parsed extensions. Image skipped." << std::endl;
                                        }

                                        continue;

                                    }

                                    if (sideProcParams && skipIfNoSidecar) {
                                        // look for the sidecar proc params
                                        if (!Glib::file_test (fileName + paramFileExtension, Glib::FILE_TEST_EXISTS)) {
                                            std::cout << "\"" << fileName << "\"  has no side-car file. Image skipped." << std::endl;
                                            continue;
                                        }
                                    }

                                    inputFiles.emplace_back (fileName);
                                }

                            } catch (Glib::Exception&) {}

                            continue;
                        }

                        std::cerr << "\"" << argument << "\" is neither a regular file nor a directory." << std::endl;
                    }

                    break;

                case 'h':
                case '?':
                default:
                    ART_print_help(argv[0], false);
                    return -1;
            }
        } else {
            argv1 = Glib::ustring (fname_to_utf8 (argv[iArg]));
#if ECLIPSE_ARGS
            argv1 = argv1.substr (1, argv1.length() - 2);
#endif

            if ( outputDirectory ) {
                options.savePathFolder = outputPath;
                options.saveUsePathTemplate = false;
            } else {
                options.saveUsePathTemplate = true;

                if (options.savePathTemplate.empty())
                    // If the save path template is empty, we use its default value
                {
                    options.savePathTemplate = "%p1/converted/%f";
                }
            }

            if (outputType == "jpg") {
                options.saveFormat.format = outputType;
                options.saveFormat.jpegQuality = compression;
                options.saveFormat.jpegSubSamp = subsampling;
            } else if (outputType == "tif") {
                options.saveFormat.format = outputType;
            } else if (outputType == "png") {
                options.saveFormat.format = outputType;
            }

            break;
        }
    }

    if (bits == -1) {
        if (outputType == "jpg") {
            bits = 8;
        } else if (outputType == "png") {
            bits = 8;
        } else if (outputType == "tif") {
            bits = 16;
        } else {
            bits = 8;
        }
    }

    if ( !argv1.empty() ) {
        return 1;
    }

    if ( inputFiles.empty() ) {
        return 2;
    }

    if (useDefault) {
        Glib::ustring profPath = options.findProfilePath(options.defProfRaw);
        Glib::ustring fname =
            profPath == DEFPROFILE_INTERNAL ?
            DEFPROFILE_INTERNAL :
            Glib::build_filename(profPath,
                                 Glib::path_get_basename(options.defProfRaw) +
                                 paramFileExtension);
        rawParams.reset(new rtengine::procparams::FilePartialProfile(nullptr, fname));

        if (options.is_defProfRawMissing() || profPath.empty() || (profPath != DEFPROFILE_DYNAMIC && !check_partial_profile(rawParams))) {
            std::cerr << "Error: default raw processing profile not found." << std::endl;
            return -3;
        }

        profPath = options.findProfilePath(options.defProfImg);
        fname =
            profPath == DEFPROFILE_INTERNAL ?
            DEFPROFILE_INTERNAL :
            Glib::build_filename(profPath,
                                 Glib::path_get_basename(options.defProfImg) +
                                 paramFileExtension);
        imgParams.reset(new rtengine::procparams::FilePartialProfile(nullptr, fname));

        if (options.is_defProfImgMissing() || profPath.empty() || (profPath != DEFPROFILE_DYNAMIC && !check_partial_profile(imgParams))) {
            std::cerr << "Error: default non-raw processing profile not found." << std::endl;
            return -3;
        }
    }

    ConsoleProgressListener cpl(inputFiles.size()+1);
    rtengine::ProgressListener *pl = progress ? &cpl : nullptr;

    if (progress) {
        const auto monitor =
            [&]() -> void
            {
                while (true) {
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    cpl.ping();
                }
            };
        std::thread(monitor).detach();
    }

    for ( size_t iFile = 0; iFile < inputFiles.size(); iFile++) {
        cpl.incr();

        // Has to be reinstanciated at each profile to have a ProcParams object with default values
        rtengine::procparams::ProcParams currentParams;

        Glib::ustring inputFile = inputFiles[iFile];
        cpl.info(Glib::ustring::compose("Output is %1-bit %2.", bits, (isFloat ? "floating-point" : "integer")));
        if (progress) {
            cpl.msg(Glib::ustring::compose("Processing: %1 (%2/%3)", inputFile, iFile+1, inputFiles.size()));
        } else {
            cpl.info(Glib::ustring::compose("Processing: %1", inputFile));
        }
        
        rtengine::InitialImage* ii = nullptr;
        rtengine::ProcessingJob* job = nullptr;
        int errorCode;
        bool isRaw = false;

        Glib::ustring outputFile;

        if ( outputType.empty() ) {
            outputType = "jpg";
        }

        if ( outputPath.empty() ) {
            Glib::ustring s = inputFile;
            Glib::ustring::size_type ext = s.find_last_of ('.');
            outputFile = s.substr (0, ext) + "." + outputType;
        } else if ( outputDirectory ) {
            Glib::ustring s = Glib::path_get_basename ( inputFile );
            Glib::ustring::size_type ext = s.find_last_of ('.');
            outputFile = Glib::build_filename(outputPath, s.substr (0, ext) + "." + outputType);
        } else {
            if (leaveUntouched) {
                outputFile = outputPath;
            } else {
                Glib::ustring s = outputPath;
                Glib::ustring::size_type ext = s.find_last_of ('.');
                outputFile = s.substr (0, ext) + "." + outputType;
            }
        }

        if ( inputFile == outputFile) {
            cpl.error(Glib::ustring::compose("cannot overwrite: %1", inputFile));
            continue;
        }

        if (!overwriteFiles && Glib::file_test(outputFile, Glib::FILE_TEST_EXISTS ) ) {
            cpl.error(Glib::ustring::compose("%1 already exists: use -Y option to overwrite. This image has been skipped.", outputFile));
            continue;
        }

        // Load the image
        isRaw = true;
        Glib::ustring ext = getExtension(inputFile);

        if (ext.lowercase() == "jpg" || ext.lowercase() == "jpeg" || ext.lowercase() == "tif" || ext.lowercase() == "tiff" || ext.lowercase() == "png") {
            isRaw = false;
        }

        ii = rtengine::InitialImage::load(inputFile, isRaw, &errorCode, nullptr);

        if (!ii) {
            errors++;
            cpl.error(Glib::ustring::compose("impossible to load file: %1", inputFile));
            continue;
        }

        if (useDefault) {
            if (isRaw) {
                if (options.defProfRaw == DEFPROFILE_DYNAMIC) {
                    rawParams = ProfileStore::getInstance()->loadDynamicProfile (ii->getMetaData());
                }
                
                cpl.info("Merging default raw processing profile.");
                rawParams->applyTo(currentParams);
            } else {
                if (options.defProfImg == DEFPROFILE_DYNAMIC) {
                    imgParams = ProfileStore::getInstance()->loadDynamicProfile (ii->getMetaData());
                }

                cpl.info("Merging default non-raw processing profile.");
                imgParams->applyTo(currentParams);
            }
        }

        bool sideCarFound = false;
        unsigned int i = 0;

        // Iterate the procparams file list in order to build the final ProcParams
        do {
            if (sideProcParams && i == sideCarFilePos) {
                // using the sidecar file
                Glib::ustring sideProcessingParams = inputFile + paramFileExtension;

                // the "load" method don't reset the procparams values anymore, so values found in the procparam file override the one of currentParams
                if (!Glib::file_test(sideProcessingParams, Glib::FILE_TEST_EXISTS) || currentParams.load(nullptr, sideProcessingParams)) {
                    cpl.info(Glib::ustring::compose("Warning: sidecar file requested but not found for: %1", sideProcessingParams));
                } else {
                    sideCarFound = true;
                    cpl.info("Merging sidecar procparams.");
                }
            }

            if (processingParams.size() > i) {
                cpl.info(Glib::ustring::compose("Merging procparams #%1", i));
                processingParams[i]->applyTo(currentParams);
            }

            i++;
        } while (i < processingParams.size() + (sideProcParams ? 1 : 0));

        if (sideProcParams && !sideCarFound && skipIfNoSidecar) {
            delete ii;
            errors++;
            cpl.error(Glib::ustring::compose("no sidecar procparams found for: %1", inputFile));
            continue;
        }

        job = create_processing_job(ii, currentParams, fast_export);

        if (!job) {
            errors++;
            cpl.error(Glib::ustring::compose("impossible to create processing job for: %1", inputFile));
            ii->decreaseRef();
            continue;
        }

        // Process image
        rtengine::IImagefloat *resultImage = rtengine::processImage(job, errorCode, pl);

        if (!resultImage) {
            errors++;
            cpl.error(Glib::ustring::compose("failure in processing: %1", inputFile));
            rtengine::ProcessingJob::destroy(job);
            continue;
        }

        // save image to disk
        if (outputType == "jpg") {
            errorCode = resultImage->saveAsJPEG(outputFile, compression, subsampling);
        } else if (outputType == "tif") {
            errorCode = resultImage->saveAsTIFF(outputFile, bits, isFloat, compression == 0);
        } else if (outputType == "png") {
            errorCode = resultImage->saveAsPNG(outputFile, bits);
        } else {
            errorCode = resultImage->saveToFile(outputFile);
        }

        if (errorCode) {
            errors++;
            cpl.error(Glib::ustring::compose("failure in saving to: %1", outputFile));
        } else {
            if (copyParamsFile) {
                Glib::ustring outputProcessingParams = outputFile + paramFileExtension;
                currentParams.save(nullptr, outputProcessingParams);
            }
        }

        ii->decreaseRef();
        resultImage->free();
    }

    if (progress) {
        std::cout << "100" << std::endl;
    }

    return errors > 0 ? -2 : 0;
}
