#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <openssl/sha.h>
#include <zlib.h>
#include <sys/stat.h>
#include <cstring>
#include <set>
#include <ctime>
#include <map>
#include <iomanip>
using namespace std;
namespace fs = filesystem;

class MyGit
{
private:
    const string GIT_DIR = ".mygit";
    const string OBJECTS_DIR = (fs::current_path() / GIT_DIR / "objects").string();

    // Helper function to create directory if it doesn't exist
    bool createDirectory(const string &path)
    {
        try
        {
            if (!fs::exists(path))
            {
                return fs::create_directories(path);
            }
            return true;
        }
        catch (const exception &e)
        {
            cerr << "Error creating directory: " << e.what() << endl;
            return false;
        }
    }

    // Helper function to compute SHA1 hash
    string computeSHA1(const string &content)
    {
        unsigned char hash[SHA_DIGEST_LENGTH];
        SHA_CTX sha1;
        SHA1_Init(&sha1);
        SHA1_Update(&sha1, content.c_str(), content.length());
        SHA1_Final(hash, &sha1);

        stringstream ss;
        for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
        {
            ss << hex << setw(2) << setfill('0') << (int)hash[i];
        }
        return ss.str();
    }

    // Helper function to compress data
    string compressData(const string &data)
    {
        z_stream zs;
        memset(&zs, 0, sizeof(zs));

        if (deflateInit(&zs, Z_BEST_COMPRESSION) != Z_OK)
        {
            throw runtime_error("deflateInit failed");
        }

        zs.next_in = (Bytef *)data.data();
        zs.avail_in = data.size();

        int ret;
        char outbuffer[32768];
        string compressed;

        do
        {
            zs.next_out = reinterpret_cast<Bytef *>(outbuffer);
            zs.avail_out = sizeof(outbuffer);

            ret = deflate(&zs, Z_FINISH);

            if (compressed.size() < zs.total_out)
            {
                compressed.append(outbuffer, zs.total_out - compressed.size());
            }
        } while (ret == Z_OK);

        deflateEnd(&zs);
        return compressed;
    }

    // Helper function to decompress data
    string decompressData(const string &data)
    {
        z_stream zs;
        memset(&zs, 0, sizeof(zs));

        if (inflateInit(&zs) != Z_OK)
        {
            throw runtime_error("inflateInit failed");
        }

        zs.next_in = (Bytef *)data.data();
        zs.avail_in = data.size();

        char outbuffer[32768];
        string decompressed;

        do
        {
            zs.next_out = reinterpret_cast<Bytef *>(outbuffer);
            zs.avail_out = sizeof(outbuffer);

            int ret = inflate(&zs, 0);

            if (decompressed.size() < zs.total_out)
            {
                decompressed.append(outbuffer, zs.total_out - decompressed.size());
            }

            if (ret == Z_STREAM_END)
                break;
            if (ret != Z_OK)
            {
                inflateEnd(&zs);
                throw runtime_error("inflate failed");
            }
        } while (true);

        inflateEnd(&zs);

        // cout<<"decopmressed data is "<<decompressed<<endl;
        return decompressed;
    }

    // Helper function to write object to storage
    string writeObject(const string &content, const string &type)
    {
        string header = type + " " + to_string(content.length()) + "$";
        // header.push_back('$');
        string store = header + content;
        string sha = computeSHA1(store);

        // cout<<" I am adding $\n";

        string compressed = compressData(store);
        string objectPath = OBJECTS_DIR + "/" + sha.substr(0, 2);
        createDirectory(objectPath);
        //  cout<<"compressed data is "<< compressed<<endl;
        string filepath = objectPath + "/" + sha.substr(2);
        ofstream file(filepath, ios::binary);
        if (file.is_open())
        {
            file.write(compressed.c_str(), compressed.length());
            file.close();
        }
        // cout<<"sha of file "<<filepath<<" is "<<sha<<endl;
        return sha;
    }

    // Helper function to read object from storage
    pair<string, string> readObject(const string &sha)
    {
        string objectPath = OBJECTS_DIR + "/" + sha.substr(0, 2) + "/" + sha.substr(2);
        // cout<<"objectpath="<<objectPath<<endl;
        ifstream file(objectPath, ios::binary);
        //  cout<<"objectPath "<<objectPath<<endl;
        if (!file.is_open())
        {
            throw runtime_error("Object not found: " + sha);
        }

        stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        string decompressed = decompressData(buffer.str());

        size_t nullPos = decompressed.find('$');
        if (nullPos == string::npos)
        {
            throw runtime_error("Invalid object format");
        }

        string header = decompressed.substr(0, nullPos);
        string content = decompressed.substr(nullPos + 1);

        size_t spacePos = header.find(' ');
        string type = header.substr(0, spacePos);

        return {type, content};
    }

    // Helper function to join lines with a separator
    string joinLines(const vector<string> &lines, size_t start, size_t end)
    {
        string result;
        for (size_t i = start; i < end; ++i)
        {
            result += lines[i];
            if (i < end - 1)
            {
                result += "\n";
            }
        }
        return result;
    }

    // Helper function to parse timestamp
    time_t parseTimestamp(const string &timestampStr)
    {
        try
        {
            return stol(timestampStr);
        }
        catch (const invalid_argument &e)
        {
            cerr << "Error: Invalid timestamp format in commit object." << endl;
            return 0;
        }
    }

    // Helper function to retrieve the tree from a given commit SHA
    map<string, string> getTreeFromCommit(const string &commitSha)
    {
        // cout<<"in get tree from commit\n";
        map<string, string> fileToSha;
        // ifstream commitFile(GIT_DIR + "/objects/" + commitSha.substr(0,2)+ "/" + commitSha.substr(2));
        string line;
        bool inTree = false;

        // cout<<"path is "<<GIT_DIR + "/objects/" + commitSha.substr(0,2)+ "/" + commitSha.substr(2)<<endl;

        string content = readObject(commitSha).second;
        // cout<<"content is "<<content<<endl;

        istringstream iss(content);
        while (iss)
        {
            // iss>>;
            // iss>>content;
            string mode, sha, filename, type;
            iss >> mode >> type >> sha >> filename;

            // cout<<"mode"<<mode<<" sha"<<sha<<" fname"<<filename<<endl;
            fileToSha[filename] = sha;
            // cout<<filename<<" "<<sha<<" "<<"done"<<endl;
        }
        // while (getline(commitFile, line)) {
        //     if (line.find("tree") == 0) {
        //         cout<<"find tree ";
        //         inTree = true;
        //         continue;
        //     }
        //     if (inTree && line.empty()) break;

        //     if (inTree) {
        //         istringstream iss(line);
        //         string mode, sha, filename;
        //         iss >> mode >> sha >> filename;
        //         fileToSha[filename] = sha;

        //         cout<<filename<<" "<<sha<<" ";
        //     }
        // }

        // cout<<endl;
        return fileToSha;
    }

    // Function to retrieve SHA-1 hashes from the index
    map<string, string> getIndexFileEntries()
    {
        map<string, string> indexEntries;
        ifstream indexFile(GIT_DIR + "/index");
        string line;
        while (getline(indexFile, line))
        {
            istringstream iss(line);
            string mode, sha, filename;
            iss >> mode >> sha >> filename;
            indexEntries[filename] = sha;
        }
        return indexEntries;
    }

public:
    // Initialize repository
    bool init()
    {
        if (fs::exists(GIT_DIR))
        {
            cerr << "Repository already exists" << endl;
            return false;
        }

        bool success = createDirectory(GIT_DIR) && createDirectory(OBJECTS_DIR);
        if (success)
        {
            cout << "Initialized empty MyGit repository in " << fs::absolute(GIT_DIR) << endl;
        }
        return success;
    }

    // Hash object command
    string hashObject(const string &filepath, bool write = false)
    {
        ifstream file(filepath, ios::binary);
        if (!file.is_open())
        {
            throw runtime_error("Cannot open file: " + filepath);
        }

        stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        string content = buffer.str();
        // cout<<"content is "<<content<<endl;
        string sha = computeSHA1("blob " + to_string(content.length()) + "$" + content);

        if (write)
        {
            // cout<<"writing blob oject having content "<<content<<endl;
            writeObject(content, "blob");
        }
        // cout<<"sha of file "<<filepath<<" is "<<sha<<endl;
        return sha;
    }

    // Cat file command
   void catFile(const string &sha, char flag)
    {
        try
        {
            auto [type, content] = readObject(sha);

            switch (flag)
            {
            case 'p':
                cout << content << "\n";
                break;
            case 't':
                cout << type << endl;
                break;
            case 's':
                cout << content.length() << endl;
                break;
            default:
                throw runtime_error("Invalid flag");
            }
        }
        catch (const exception &e)
        {
            cerr << "Error: " << e.what() << endl;
        }
        


    }

    // Write tree command
    string writeTree()
    {
        // cout << "coming to write tree\n";
        stringstream treeContent;

        // Iterate through the current directory
        for (const auto &entry : fs::directory_iterator("."))
        {
            // cout << "entry is " << entry.path() << endl;

            if (entry.path().filename().string() == GIT_DIR or entry.path().filename().string() == ".git")
                continue; // Skip the .mygit directory

            string name = entry.path().filename().string();
            string mode;
            string sha;
            string objectType;

            if (fs::is_directory(entry))
            {
                // Directory: change path to the subdirectory
                mode = "040000";
                string currentPath = fs::current_path().string(); // Store current path
                fs::current_path(entry);                          // Change to the subdirectory

                sha = writeTree(); // Recursive call to handle the subdirectory

                fs::current_path(currentPath); // Change back to the original path after recursion
                objectType = "tree";
            }
            else
            {
                // File: calculate its SHA-1 and store it as a blob
                mode = "100644";
                sha = hashObject(name, true);
                objectType = "blob";
            }

            // Write entry as: mode objectType sha name
            treeContent << mode << " " << objectType << " " << sha << " " << name << "\n";
        }

        // Return the SHA of the tree object
        return writeObject(treeContent.str(), "tree");
    }

    // Add this new method to the public section of the MyGit class

    void listTree(const string &sha, bool nameOnly = false)
    {
        try
        {
            auto [type, content] = readObject(sha); // Read the tree object using its SHA

            if (type != "tree")
            {
                throw runtime_error("Object is not a tree");
            }

            istringstream stream(content);
            string entry;

            // Parse each entry in the tree content (mode, object type, sha, name)
            while (getline(stream, entry))
            {
                if (entry.empty())
                    continue;

                istringstream entryStream(entry);
                string mode, objectType, objectSha, name;
                entryStream >> mode >> objectType >> objectSha >> name;

                if (nameOnly)
                {
                    cout << name << endl; // Print only names if --name-only flag is used
                }
                else
                {
                    cout << mode << " " << objectType << " " << objectSha << " " << name << endl; // Print full details
                }
            }
        }
        catch (const exception &e)
        {
            cerr << "Error: " << e.what() << endl;
        }
    }

    // Function to add files to the index
    void addFiles(const vector<string> &files)
    {
        set<string> addedFiles;         // To track filenames already added to the index
        map<string, string> fileHashes; // To track filename and their last stored hashes

        // Open index file in read mode to load existing contents
        ifstream indexRead(GIT_DIR + "/index");
        string line;
        while (getline(indexRead, line))
        {
            istringstream iss(line);
            string mode, sha, filename;
            iss >> mode >> sha >> filename;
            addedFiles.insert(filename);
            fileHashes[filename] = sha; // Store the existing hash for comparison
        }
        indexRead.close();

        ofstream indexFile(GIT_DIR + "/index", ios::app); // Append mode for new entries
        for (const string &file : files)
        {
            // Skip adding directories
            if (fs::is_directory(file))
                continue;

            // Generate the new SHA-1 hash
            string newSha = hashObject(file, true);

            // Check if file is already indexed and if the hash has changed
            if (addedFiles.find(file) != addedFiles.end() && fileHashes[file] == newSha)
            {
                continue; // Skip if the file's hash is the same as before
            }

            // Write the file's details (mode, SHA, filename) to the index
            string mode = "100644"; // Regular file mode
            indexFile << mode << " " << newSha << " " << file << endl;
            addedFiles.insert(file); // Add to the set to prevent duplicates
            cout << "Added " << file << " to the index." << endl;
        }
        indexFile.close();
    }

    // end

    // adding updated commit

    // Updated commitChanges function to display the number of changed files
    void commitChanges(const string &message = "")
    {
        try
        {
            // 1. Validate staging area
            string indexPath = GIT_DIR + "/index";
            if (!fs::exists(indexPath))
            {
                throw runtime_error("Nothing to commit (create/copy files and use 'mygit add' to track)");
            }

            // 2. Retrieve parent commit's SHA from HEAD
            string parentCommit = readHead(); // This might be empty for the first commit
            // cout << "DEBUG: parentCommit: '" << parentCommit << "'" << endl;  // Debugging

            map<string, string> parentTree;

            // 3. Check if there is a parent commit
            if (!parentCommit.empty())
            {
                // Get the path to the parent commit file
                string parentCommitPath = GIT_DIR + "/objects/" + parentCommit.substr(0, 2) + "/" + parentCommit.substr(2);
                // cout << "DEBUG: parentCommitPath: '" << parentCommitPath << "'" << endl;  // Debugging

                //    commitFile=<<endl;
                string treeSha = readObject(parentCommit).second.substr(5, 40);

                // cout<<"tree sha is"<<treeSha<<endl;

                // ifstream commitFile(readObject(parentCommit).second);
                // if (!commitFile.is_open()) {
                //     throw runtime_error("Unable to open parent commit file.");
                // }
                // cout<<"commfile is"<<commitFile.string()<<endl;

                // cout<<"line "<<endl;
                // while (getline(commitFile, line)) {
                //      cout<<"labove ine is "<<line<<endl;
                //     if (line.find("tree") == 0) {
                //         istringstream iss(line);
                //         string temp;
                //         iss >> temp >> treeSha; // extract the tree SHA
                //         // cout<<"line is "<<line<<endl;
                //         // treeSha=line.substr(5,40);
                //         break;
                //     }
                // }
                // commitFile.close();

                // cout << "DEBUG: treeSha: '" << treeSha << "'" << endl;  // Debugging

                // Check if treeSha is empty before using substr()
                if (treeSha.empty())
                {
                    throw runtime_error("Tree SHA is empty, possible corruption in the commit object.");
                }

                // Access the tree object using its SHA-1
                string treePath = GIT_DIR + "/objects/" + treeSha.substr(0, 2) + "/" + treeSha.substr(2);
                // cout << "DEBUG: treePath: '" << treePath << "'" << endl;  // Debugging
                parentTree = getTreeFromCommit(treeSha); // Retrieve the tree contents as a map
            }
            else
            {
                // No parent commit, this is the first commit
                cout << "This is the first commit." << endl;
            }

            // 4. Retrieve current index entries
            map<string, string> currentIndex = getIndexFileEntries();
            int changedFilesCount = 0;
            //  cout<<"currrnt ="<<
            // 5. Compare index with the parent tree
            for (const auto &[filename, sha] : currentIndex)
            {
                // If the file is new (not in the parent tree) or modified (SHA differs)
                if (parentTree.find(filename) == parentTree.end() || parentTree[filename] != sha)
                {
                    changedFilesCount++;
                }
            }

            // 6. If no changes are detected, print a message and return
            if (changedFilesCount == 0)
            {
                cout << "0 changes to commit" << endl;
                // return;
            }

            // 7. Create and write the commit object with metadata
            string commitMsg = message.empty() ? "Default commit message" : message;
            stringstream commitContent;
            commitContent << "tree " << createTreeFromIndex() << "\n";
            if (!parentCommit.empty())
                commitContent << "parent " << parentCommit << "\n";
            commitContent << "author " << getAuthorInfo() << " " << getTimestamp() << "\n";
            commitContent << "committer " << getAuthorInfo() << " " << getTimestamp() << "\n\n";
            commitContent << commitMsg << "\n";
            string commitSha = writeObject(commitContent.str(), "commit");

            // 8. Update HEAD and clear index
            updateHead(commitSha);
            ofstream indexFile(indexPath, ios::trunc); // Clear index after commit
            indexFile.close();

            // 9. Output success message with changed files count
            cout << "[main " << commitSha.substr(0, 7) << "] " << commitMsg << "\n";
            cout << "Files changed: " << changedFilesCount << endl;
        }
        catch (const exception &e)
        {
            cerr << "Error: " << e.what() << endl;
        }
    }

    // Helper function to create a tree object from index
    string createTreeFromIndex()
    {
        vector<string> stagedFiles;
        string indexPath = GIT_DIR + "/index";
        ifstream indexFile(indexPath);

        if (!indexFile.is_open())
        {
            throw runtime_error("Index file not found");
        }

        string line;
        while (getline(indexFile, line))
        {
            if (!line.empty())
            {
                stagedFiles.push_back(line);
            }
        }
        indexFile.close();

        if (stagedFiles.empty())
        {
            throw runtime_error("No files in staging area");
        }

        // Write tree from staged files directly
        return writeTreeFromStagedFiles(stagedFiles);
    }
    //  static int countin=0;
    // Helper function to create tree object from staged files
    string writeTreeFromStagedFiles(const vector<string> &stagedFiles)
    {
        stringstream treeContent;

        // Sort files to ensure consistent tree hashes
        map<string, pair<string, string>> sortedEntries; // filename -> (mode, sha)
        int count = 0;
        for (const string &line : stagedFiles)
        {
            istringstream iss(line);
            string mode, sha, filename;
            iss >> mode >> sha >> filename;
            sortedEntries[filename] = make_pair(mode, sha);
            count++;
        }

        cout << count << " files wass there in staging area" << endl;

        // Build tree content from sorted entries
        for (const auto &entry : sortedEntries)
        {
            treeContent << entry.second.first << " blob " << entry.second.second
                        << " " << entry.first << "\n";
        }

        // Write and return tree object
        return writeObject(treeContent.str(), "tree");
    }

    // Helper function to read HEAD commit
    string readHead()
    {
        string headPath = GIT_DIR + "/HEAD";
        ifstream headFile(headPath);
        string head;

        if (headFile.is_open())
        {
            getline(headFile, head);
            headFile.close();
            return head;
        }
        return "";
    }

    // Helper function to update HEAD
    void updateHead(const string &commitSha)
    {
        string headPath = GIT_DIR + "/HEAD";
        ofstream headFile(headPath);
        if (headFile.is_open())
        {
            headFile << commitSha;
            headFile.close();
        }
        else
        {
            throw runtime_error("Could not update HEAD");
        }
    }

    // Helper function to get current timestamp as string

    string getTimestamp()
    {
        time_t now = time(nullptr);
        return to_string(now);
    }

    // Helper function to get commit author info
    string getAuthorInfo()
    {
        return "Saurav <sauravdeshmukh200@gmail.com>";
    }

    // Helper function to count staged files
    int countStagedFiles()
    {
        int count = 0;
        ifstream indexFile(GIT_DIR + "/index");
        string line;
        while (getline(indexFile, line))
        {
            if (!line.empty())
            {
                count++;
            }
        }
        return count;
    }

    void logCommits()
    {

        // cout<<"printing path "<<OBJECTS_DIR<<endl;
        string headCommit;
        ifstream headFile(GIT_DIR + "/HEAD");
        if (headFile.is_open())
        {
            getline(headFile, headCommit);
            headFile.close();
        }
        else
        {
            cerr << "Error: Failed to read HEAD file." << endl;
            return;
        }

        while (!headCommit.empty())
        {
            auto [type, content] = readObject(headCommit);
            if (type != "commit")
            {
                cerr << "Error: HEAD points to a non-commit object." << endl;
                return;
            }

            istringstream commitStream(content);
            string line;
            string parentCommit;
            string author, committer, message;
            time_t timestamp = 0;

            // Parse the commit object
            while (getline(commitStream, line))
            {
                if (line.empty())
                {
                    // Commit message starts after the empty line
                    getline(commitStream, message);
                    break;
                }
                else if (line.find("tree") == 0)
                {
                    // Ignore the tree line
                }
                else if (line.find("parent") == 0)
                {
                    // Extract the parent commit SHA
                    parentCommit = line.substr(7);
                }
                // else if (line.find("author") == 0)
                // {
                //     // Extract the author information
                //     author = line.substr(7); // Skip "author " (7 characters)

                //     // Find the position of the last space, which is before the timestamp
                //     size_t lastSpacePos = author.find_last_of(' ');
                //     if (lastSpacePos != string::npos)
                //     {
                //         string timestampStr = author.substr(lastSpacePos + 1); // Get the timestamp part
                //         author = author.substr(0, lastSpacePos);               // Get the author email part

                //         // Convert timestamp to integer
                //         try
                //         {
                //             timestamp = stol(timestampStr);
                //         }
                //         catch (const invalid_argument &e)
                //         {
                //             cerr << "Error: Invalid timestamp format in commit object." << endl;
                //         }
                //     }
                // }
                else if (line.find("committ") == 0)
                {
                    // Extract the committer information
                    committer = line.substr(10);
                    string temp;
                    int n=committer.size();
                    for(int i=n-1;i>=0 and committer[i]!=' ';i--)
                    {
                        temp.push_back(committer[i]);
                    }

                    reverse(temp.begin(),temp.end());

                    timestamp=stoi(temp);
                }
            }

            // Print the commit information
            cout << "commit " << headCommit << endl;
            if (!parentCommit.empty())
            {
                cout << "parent " << parentCommit << endl;
            }
            // cout << "Author: " << author << endl;
            cout<<"commit message :"<< message << endl;

            // Format and print the timestamp
            if (timestamp != 0)
            {
                tm *gmt = gmtime(&timestamp); // Get UTC time
                stringstream ss;
                ss << put_time(gmt, "%Y-%m-%d %H:%M:%S UTC");
                cout<<"date and time :"<<ss.str()<<endl;
            }
            else
            {
                cout << " (invalid timestamp)" << endl;
            }

            cout<<"commiter info: "<<committer<<endl<<endl;

            // Move to the parent commit
            headCommit = parentCommit;
        }
    }

    void checkout(const string& commitSHA) {
    string treeSHA = getTreeSHA(commitSHA);
    // if (treeSHA.empty()) {
    //     cerr << "Error: Invalid commit SHA." << endl;
    //     return;
    // }

    vector<tuple<string, string, string, string>> treeEntries = parseTree(treeSHA);
    restoreFiles(treeEntries);

    cout << "Checked out commit " << commitSHA << endl;
}


// void restoreFiles(const vector<pair<string, string>>& treeEntries) {
//     for (const auto& entry : treeEntries) {
//         string typeAndSHA = entry.first;
//         string filename = entry.second;

//         if (typeAndSHA.find("blob") != string::npos) {
//             string fileSHA = typeAndSHA.substr(5);
//             string filePath = ".mygit/objects/" + fileSHA;

//             ifstream blobFile(filePath, ios::binary);
//             ofstream restoredFile(filename, ios::binary);

//             restoredFile << blobFile.rdbuf();
//         } else if (typeAndSHA.find("tree") != string::npos) {
//             string dirSHA = typeAndSHA.substr(5);
//             string dirName = filename;
//             mkdir(dirName.c_str(), 0755); // Create directory
//             restoreFiles(parseTree(dirSHA)); // Recursively restore subdirectories
//         }
//     }
// }


void restoreFiles(const vector<tuple<string, string, string, string>>& treeEntries) {
    for (const auto& entry : treeEntries) {
        string mode, objectType, sha, name;
        tie(mode, objectType, sha, name) = entry;

        if (objectType == "blob") { // File
            pair<string, string> blobData = readObject(sha);

            if (blobData.first != "blob") {
                cerr << "Error: Object is not of type 'blob' for SHA " << sha << endl;
                continue;
            }

            // // Skip the header in the blob
            // size_t headerEnd = blobData.second.find('$');
            // if (headerEnd == string::npos) {
            //     cerr << "Error: Invalid blob object format for SHA " << sha << endl;
            //     continue;
            // }
            string blobContent = blobData.second;
            // Ensure parent directories exist
            createDirectories(name);

            // Write blob content to file
            ofstream restoredFile(name, ios::binary);
            restoredFile << blobContent;
            restoredFile.close();

        } else if (objectType == "tree") { // Directory
            mkdir(name.c_str(), 0755); // Create directory
            restoreFiles(parseTree(sha)); // Recursively restore subdirectories
        }
    }
}



vector<tuple<string, string, string, string>> parseTree(const string& treeSHA) {
    // Use readObject to fetch the type and content of the tree object
    pair<string, string> objectData = readObject(treeSHA);
    string type = objectData.first;
    string content = objectData.second;

    if (type != "tree") { // Ensure the object type is "tree"
        cerr << "Error: Object is not of type 'tree' for SHA " << treeSHA << endl;
        return {};
    }

    
    string treeContent = content;

    vector<tuple<string, string, string, string>> entries; // Store (mode, objectType, sha, name)

    stringstream contentStream(treeContent);
    string line;
    while (getline(contentStream, line)) {
        stringstream lineStream(line);
        string mode, objectType, sha, name;

        if (!(lineStream >> mode >> objectType >> sha >> name)) {
            cerr << "Error: Malformed tree entry in SHA " << treeSHA << endl;
            continue;
        }

        entries.emplace_back(mode, objectType, sha, name);
    }

    return entries;
}

string getTreeSHA(const string& commitSHA) {
    // Use readObject to fetch the type and content of the commit file
    pair<string, string> objectData = readObject(commitSHA);
    string type = objectData.first;
    string content = objectData.second;

    if (type != "commit") { // Ensure the object type is "commit"
        cerr << "Error: Object is not of type 'commit' for SHA " << commitSHA << endl;
        return "";
    }

    stringstream contentStream(content);
    string line, treeSHA;

    while (getline(contentStream, line)) {          // Read each line of the content
        if (line.rfind("tree ", 0) == 0) {          // Check if the line starts with "tree "
            treeSHA = line.substr(5);               // Extract everything after "tree "
            break;
        }
    }

    if (treeSHA.empty()) {
        cerr << "Error: Tree SHA not found in commit object for SHA " << commitSHA << endl;
    }
  

    cout<<"tree sha is"<< treeSHA<<endl;
    return treeSHA;                                 // Return the extracted tree SHA
}


void createDirectories(const string& filePath) {
    size_t lastSlash = filePath.find_last_of('/');
    if (lastSlash == string::npos) return; // No directories in the path

    string dirPath = filePath.substr(0, lastSlash); // Extract directory path
    stringstream ss(dirPath);
    string segment;
    string currentPath = ".";

    while (getline(ss, segment, '/')) { // Create each segment of the path
        currentPath += "/" + segment;
        mkdir(currentPath.c_str(), 0755); // Create directory if it doesn't exist
    }
}


};