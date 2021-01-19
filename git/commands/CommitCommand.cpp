#include "CommitCommand.hpp"
#include "../common.hpp"
#include "../objects/GitTree.hpp"
#include "../objects/GitCommit.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>

typedef boost::tokenizer<boost::char_separator<char>> tokenizer;

namespace fs = boost::filesystem;
using namespace std;

CommitCommand::CommitCommand(int argc, char* argv[])
{
    numArgs = argc;
    args = argv;
}
CommitCommand::~CommitCommand()
{
    
}

int CommitCommand::execute()
// Executes command. Returns non-zero if failure to do so.
{
    //1. Verify that a git folder has been initiated
    if (!fs::exists("./.git/") || !fs::is_directory("./.git/"))
    {
        cout << "Error: No git repository has been found.\n";
        return 1;
    }
    //2. Argument verification
    if(numArgs != 4)
    {
        cout << "Error: Was expecting 2 arguments. Got " << to_string(numArgs - 2)<<"."  <<endl;
        help();
        return 1;
    }
    commitMessage = args[2];
    commitAuthor = args[3];

    //3. Verify that there are staged files
    if(readFile(".git/index").size() == 0)
    {
        cout << "Error: There are no staged files. You can stage some files using ./gitus add <pathspec>\n";
        return 1;
    }

    //3. Fetch root tree
    GitTree *root = nullptr;
    string parentCommitSHA1 = readFile(".git/HEAD");
    if(parentCommitSHA1.size() != 0)
    {
        GitCommit* parentCommit = GitCommit::createFromGitObject(parentCommitSHA1);
        root = parentCommit->getRootTree();
    }
    else root = new GitTree();

    //4. Read the index file.
    fs::path indexPath("./.git/index");
    string indexContents = readFile(indexPath.c_str());

    //5. For each line in the index file, insert blob into root tree.
    boost::char_separator<char> sep{"\n"};
    tokenizer tokenized{indexContents, sep};
    for(const auto& token: tokenized)
    {
        //Split line of index file into its filename and hash components according to delimeter. 
        stringstream fileref(token);
        string filePath,fileHash;
        getline(fileref,filePath,INDEX_FILE_DEMLIMETER);
        getline(fileref,fileHash,INDEX_FILE_DEMLIMETER);

        //Adding the versioned file to the GitTree
        root->addBlob(filePath, fileHash);
    }

    //6. Sort root tree to produce deterministic results
    root->sort();

    //7. Generate the tree SHA1 hash;
    root->generateTreeSHA1();

    //8. Save tree objects to the .git/objects folder
    root->addTreeInObjects(); 

    //9. Create commit object 
    GitCommit* commitobj = new GitCommit(root, &commitAuthor, &commitMessage, &parentCommitSHA1);

    //10. Save commit object to the .git/objects folder
    commitobj->generateCommitSHA1();
    commitobj->addCommitInObjects();

    //11. Delete content on .git/index file
    clearIndex();

    //12. Update the .git/HEAD file
    updateHEAD(commitobj);

    return 0;
}

void CommitCommand::help() {
    cout << "usage: gitus commit <msg> <author>" << endl;
}

void CommitCommand::clearIndex()
//Clears the content of the .index file
{
    ofstream file;
    file.open(".git/index", std::ofstream::out | std::ofstream::trunc);
    file.close();
}


void CommitCommand::updateHEAD(GitCommit* obj)
//Insert SHA1 of new commit to the HEAD file
{
    ofstream file;
    file.open(".git/HEAD", std::ofstream::out | std::ofstream::trunc);
    string commitSHA1 = obj->getSHA1Hash();
    file.write(commitSHA1.c_str(), 40);
    file.close();
}