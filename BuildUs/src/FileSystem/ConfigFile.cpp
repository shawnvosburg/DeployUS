#include "ConfigFile.hpp"
#include <boost/algorithm/string.hpp>
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>

ConfigFile::ConfigFile(fs::path filepath)
//Can throw std::runtime_error! 
{
    this->configPath = filepath;

    //1. Verify that file exists
    auto configfilepath = fs::path(filepath);
    if(!fs::exists(configfilepath))
    {
        string msg = string("Error: File ") + filepath.string() + string(" does not exists.");
        throw std::runtime_error(msg);
    }

    //2. Read files
    std::stringstream configcontents = readFile(configfilepath);

    //3. Initialize members
    initialize(configcontents);
}

ConfigFile::ConfigFile(fs::path simulatedConfigPath, std::stringstream& bytestream)
//Create Config from bytestream. Usefull for unittesting
{
    this->configPath = simulatedConfigPath;
    initialize(bytestream);
}

void ConfigFile::initialize(std::stringstream& bytestream)
//Common code to all constructors
{
    //1. Parsing. Sets config attribute
    this->parseYAML(bytestream);

    //2. Verify that config is valid
    if(this->isYAMLInvalid())
    {
        string msg = string("Error: Configuration file is not valid.");
        throw std::runtime_error(msg);
    }

    //3. Verify that all compilation units exists
    //   Note: This function can throw std::runtime_error
    this->verifyCompilationUnitsExists();
}

ConfigFile* ConfigFile::safeFactory(fs::path filepath)
//Catches all errors. Returns nulltpr if an error occured
{
    ConfigFile* out;
    try
    {
        out = new ConfigFile(filepath);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        out = nullptr;
    }
    return out;
}

static ConfigFile* safeFactory(fs::path simulatedConfigPath, std::stringstream& bytestream)
//Catches all error. Returns nullptr if an error occured
{
    ConfigFile* out;
    try
    {
        out = new ConfigFile(simulatedConfigPath, bytestream);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        out = nullptr;
    }
    return out;
}

ConfigFile::~ConfigFile()
{
}

void ConfigFile::parseYAML(std::stringstream& bytestream)
/*
    Fills attribut fields with contents of config file.
*/
{
    //Saving YAML config
    this->config = YAML::Load(bytestream);

    //Filling attributes
    this->projectName = ConfigFileUtils::vectorizeYAMLNode(this->config[CONFIG_FILE_PROJECT]);
    this->compileList = ConfigFileUtils::generateCompileList(this->config);
    this->depLibVars =  ConfigFileUtils::vectorizeYAMLNode(this->config[CONFIG_FILE_DEP_LIBRARY][CONFIG_FILE_VARS]);
    this->depLibList =  ConfigFileUtils::vectorizeYAMLNode(this->config[CONFIG_FILE_DEP_LIBRARY][CONFIG_FILE_LIBS]);
    this->depInclVars = ConfigFileUtils::vectorizeYAMLNode(this->config[CONFIG_FILE_DEP_INCL][CONFIG_FILE_VARS]);
}

void ConfigFile::verifyCompilationUnitsExists()
// Checks to see if all filepaths from compile lists exists.
// Throws runtime error if it is the case.
{
    for(auto elem: this->getCompileList())
    {
        fs::path compileunitPath = this->getConfigPath().parent_path().append(elem.second);
        if(!fs::exists(compileunitPath))
        {
            string msg = string("Error: Compilation unit ") + elem.second + string(" does not exists.");
            throw std::runtime_error(msg);
        }
    }
}

bool const ConfigFile::isYAMLInvalid()
/*
    Returns true if the YAML file is invalid.
    Returns false if it is a valid YAML file
*/
{
    //Only allowed 1 project name
    if(this->getProjectName().size() != 1)
        return true;

    //Must compile at least 1 file
    if(this->getCompileList().size() < 1)
        return true;

    return false;
}


string const ConfigFile::toString()
// Returns string representation of fields.
// Mainly a debugging function, in order to visualize the fields of the ConfigFile class.
// Should not be used during normal executable operation.
{
    return ConfigFileUtils::createConfigContents(getProjectName(),getCompileList(),getDepLibVars(),getDepLibList(),getDepInclVars()).str();
}

/*
    ====================================
    Helper functions definitions
    ====================================
*/

StringList const ConfigFileUtils::vectorizeYAMLNode(const YAML::Node node)
//Returns vector of string contained in Node. Must be the last containing node.
{
    StringList vectorized;
    switch (node.Type())
    {
        case YAML::NodeType::Scalar : 
            vectorized.push_back(node.as<string>());
            break;
        case YAML::NodeType::Sequence:
            for(int i = 0; i < node.size(); i++ )
                vectorized.push_back(node[i].as<string>());
            break;
        case YAML::NodeType::Map:
            //Special case. We now have two strings.
            //The map key and its value.
            //We will concatenate the two with a \0 character
            {
                std::map<string,string> nodemap = node.as<std::map<string,string>>();
                for(auto nodeItem: nodemap)
                {
                    string key   = nodeItem.first;
                    string value = nodeItem.second;
                    string concatenated = key + '\0' + value;
                    vectorized.push_back(concatenated);
                }
            }
            break;
        default:
            break;
    }
    return vectorized;
}

StringPairList const ConfigFileUtils::generateCompileList(const YAML::Node node)
//Creates list of compilation units
{
    StringPairList toCompile;
    for(int i = 0; i < node[CONFIG_FILE_COMPILE].size(); i++ )
    {
        string item = "f";
        item += std::to_string(i + 1);
        for(auto elem: ConfigFileUtils::vectorizeYAMLNode(node[CONFIG_FILE_COMPILE][i]))
        {
            int nullPos = elem.find('\0');
            string outputPath = elem.substr(0,nullPos);
            string inputPath = elem.substr(nullPos+1);
            std::pair<string, string> compileUnit(outputPath,inputPath);
            toCompile.push_back(compileUnit);
        }
    }
    return toCompile;
}

/* Creates the content of a valid YAML config file. Useful for unit testing*/
std::stringstream ConfigFileUtils::createConfigContents(    StringList      projectName,
                                                            StringPairList  compileList,
                                                            StringList      depLibVars,
                                                            StringList      depLibList,
                                                            StringList      depInclVars)
{
    std::stringstream out;
    
    //1. Project
    out << CONFIG_FILE_PROJECT << ": " << projectName.at(0) << "\n";
    
    //2. Compile List
    out << CONFIG_FILE_COMPILE << ":\n";
    for(auto compileUnit: compileList)
    {
        out << " - " << compileUnit.first <<": " << compileUnit.second << '\n';
    }

    if(depLibList.size() > 0 || depLibVars.size() > 0)
    {
        out << CONFIG_FILE_DEP_LIBRARY << ": \n";

        //3. Dependencies vars List
        if(depLibVars.size() > 0)
        {
            out << " " << CONFIG_FILE_VARS << ":\n";
            for(auto var: depLibVars)
            {
                out << "  " << var << '\n';
            }
        }

        //4. Dependencies libraries List
        if(depLibList.size() > 0)
        {   out << " " << CONFIG_FILE_LIBS << ":\n";
            for(auto libpath: depLibList)
            {
                out << " - " << libpath << '\n';
            }
        }
    }


    //5. Includes List
    if(depLibList.size() > 0)
    {
        out << CONFIG_FILE_DEP_INCL << ": \n";
        out << " " << CONFIG_FILE_VARS << ":\n";
        for(auto var: depInclVars)
        {
            out << "  " << var << '\n';
        }
    }
    
    return out;
}