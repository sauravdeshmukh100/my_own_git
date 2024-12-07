#include <iostream>
#include <string>
#include <vector>
#include <bits/stdc++.h>
#include "my_git.cpp"

void printUsage()
{
    cout << "Usage: ./mygit <command> [<args>]\n\n"
         << "Commands:\n"
         << "   init                    Create an empty MyGit repository\n"
         << "   hash-object [-w] <file> Compute object ID and optionally write the object\n"
         << "   cat-file [-p|-t|-s] <object> Show object content, type, or size\n"
         << "   write-tree              Write the working directory as a tree object\n"
         << "   ls-tree [--name-only] <tree-sha> List contents of a tree object\n"
         << "   add <file(s)>           Add file(s) to the staging area\n"
         << "   commit -m \"<msg>\"       Commit changes to the repository\n"
         << "   log                     Show commit logs\n";
}

int main(int argc, char *argv[])
{

    // cout<<"make working"<<endl;

    // cout<<"changes saved"<<endl;
    if (argc < 2)
    {
        printUsage();
        return 1;
    }

    string command = argv[1];
    MyGit git;

    // cout<<"command is "<<command<<endl;

    try
    {
        if (command == "init")
        {
            return git.init() ? 0 : 1;
        }
        else if (command == "hash-object")
        {
            if (argc < 3)
            {
                cerr << "Error: Missing file argument" << endl;
                return 1;
            }

            bool write = false;
            string filepath;

            if (string(argv[2]) == "-w")
            {
                if (argc < 4)
                {
                    cerr << "Error: Missing file argument" << endl;
                    return 1;
                }
                write = true;
                filepath = argv[3];
            }
            else
            {
                filepath = argv[2];
            }

            cout << git.hashObject(filepath, write) << endl;
        }
        else if (command == "cat-file")
        {
            if (argc < 4)
            {
                cerr << "Error: Missing flag or object argument" << endl;
                return 1;
            }

            string flag = argv[2];
            if (flag.length() != 2 || flag[0] != '-' ||
                (flag[1] != 'p' && flag[1] != 't' && flag[1] != 's'))
            {
                cerr << "Error: Invalid flag" << endl;
                return 1;
            }

            git.catFile(argv[3], flag[1]);
        }
        else if (command == "write-tree")
        {
            //     cout<<"andar aa raha hai"<<endl;
            cout << git.writeTree() << endl;
        }

        else if (command == "ls-tree")
        {
            if (argc < 3)
            {
                cerr << "Error: Missing tree object SHA" << endl;
                return 1;
            }

            bool nameOnly = false;
            string treeSha;

            if (string(argv[2]) == "--name-only")
            {
                if (argc < 4)
                {
                    cerr << "Error: Missing tree object SHA" << endl;
                    return 1;
                }
                nameOnly = true;
                treeSha = argv[3];
            }
            else
            {
                treeSha = argv[2];
            }

            git.listTree(treeSha, nameOnly);
        }
        else if (command == "add" && argc == 3 && string(argv[2]) == ".")
        {
            vector<string> files;

            // Recursively traverse the current directory
            for (const auto &entry : fs::recursive_directory_iterator("."))
            {
                // Skip files or directories inside the .mygit directory
                if (entry.path().string().find(".mygit") != string::npos or entry.path().string().find(".git") != string::npos)
                    continue;

                // Add regular files to the files vector
                if (entry.is_regular_file())
                {
                    string temp=entry.path().string();
                    temp=temp.substr(2);
                    // cout<<"file="<<temp<<endl;
                    
                    files.push_back(temp);
                }
            }

            // Pass the collected files to git.addFiles
            git.addFiles(files);
        }
        else if (command == "add")
        {
            if (argc < 3)
            {
                cerr << "No files specified for add command." << endl;
                return 1;
            }

            vector<string> files;
            for (int i = 2; i < argc; ++i)
            {
                string temp=argv[i];
                if(temp[0]=='.' and temp[1]=='/')
                {
                   temp=temp.substr(2);
                }
                files.push_back(temp);
            }

            git.addFiles(files);
        }
        else if (command == "commit")
        {
            string message; // Default commit message

            // Check if -m flag is provided
            for (int i = 2; i < argc; ++i)
            {
                if (string(argv[i]) == "-m" && i + 1 < argc)
                {
                    message = argv[i + 1];
                    break;
                }
            }

            // Create the commit
            git.commitChanges(message);
        }
        else if (command == "log")
        {
            git.logCommits();
        }
        else
        {
            cerr << "Error: Unknown command '" << command << "'" << endl;
            printUsage();
            return 1;
        }
    }
    catch (const exception &e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}