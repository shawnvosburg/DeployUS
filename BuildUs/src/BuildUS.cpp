#include "BuildUS.hpp"
#include <iostream>
#include <FileSystem/ConfigFile.hpp>
#include <FileSystem/BuildUSCache.hpp>
#include <boost/filesystem.hpp>
#include <GCCDriver/GCCDriver.hpp>

namespace fs = boost::filesystem;

namespace BuildUS
{
    //Constants
    const char* BUILDUS_EXTENSION = ".buildus";
    const char* BUILDUS_CLEAN = "clean";

    /*
        Main function. Returns non-zero if an error occurs.
    */
    int start(int argc, const char *argv[])
    {
        //Verify Arugments
        if(argc != 2)
        {
            std::cout << "Error: Wrong number of arguments.\n";
            std::cout << "Usage: ./BuildUS <Config filepath>" << std::endl;
            return 1;
        }
        fs::path arg1 = argv[1];
        
        //Special command: ./BuildUS clean
        if(arg1.compare(BUILDUS_CLEAN) == 0)
            return clean();

        //Check file extension
        if(arg1.extension().compare(BUILDUS_EXTENSION) != 0)
        {
            std::cout << "Error: Config file must have .buildus extension.\n";
            return 1;
        }


        //1. Load Config File
        ConfigFile* config = ConfigFile::safeFactory(arg1);
        if(config == nullptr)
            return 1;
        
        //2. Create GCCDriver Object
        GCCDriver* gcc = GCCDriver::safeFactory(config);
        if(gcc == nullptr)
            return 1;
        
        //3. Compiling step
        if(gcc->compile())
        {
            return 1;
        }

        //4. Linking step
        if(gcc->link())
        {
            return 1;
        }

        //Reclaim memory
        delete config;
        delete gcc;

        return 0;
    }

    int clean()
    //Removes intermediate folder
    {
        string err;
        //Deleting executable
        if(fs::exists(BUILDUS_CACHE_INTERMEDIATE_PROJECT_CACHE))
        {
            std::stringstream projectcache = readFile(BUILDUS_CACHE_INTERMEDIATE_PROJECT_CACHE);
            string execPath = BuildUSCacheUtils::getCacheToken(projectcache);
            try
            {
                fs::remove(fs::path(execPath));
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
                err += "Error: Could not remove executable.\n";
            }
            
            
        }
        
        //Deleting intermediate folder
        if(fs::exists(BUILDUS_CACHE_INTERMEDIATE_FOLDER))
        {
            try
            {
                fs::remove_all(BUILDUS_CACHE_INTERMEDIATE_FOLDER);
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
                err += "Error: Could not clean directory.\n";
            }
        }

        if(err.size() > 0)
        {
            std::cout << err;
            return 1;
        }
        return 0;
    }

}
