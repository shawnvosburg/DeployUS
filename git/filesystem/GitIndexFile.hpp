#pragma once
#include <objects/GitBlob.hpp>
#include <list>
#include <utility>
#include <string>
#include <boost/tokenizer.hpp>

#define GITINDEXFILE_INTER_SEPERATOR '\n'
#define GITINDEXFILE_INTRA_SEPERATOR '\0'

using namespace std;
typedef list<pair<string,GitBlob>> ListBlobs;
typedef boost::tokenizer<boost::char_separator<char>> tokenizer;

class GitIndexFile
{
    private:
        ListBlobs blobs;

    public:
        GitIndexFile();
        ~GitIndexFile();

        int addBlob(GitBlob blob);
        
        void produceIndexFile();
        ListBlobs parseIndexFile();

        int contains(const string filepath);

};