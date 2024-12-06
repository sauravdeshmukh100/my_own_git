Mini Version Control System (MyGit)
Introduction
This project is an implementation of a mini version control system (VCS) that mimics the basic functionalities of Git. It allows users to create a new repository, add files, commit changes, and view the commit history.


Installation and Usage

Ensure you have a C++ compiler (e.g., g++ or clang++) and the OpenSSL and zlib libraries installed on your system.
Clone the repository and navigate to the project directory.

Compile the source files using the provided makefile:
make
or
g++ main.cpp -o mygit -lssl -lcrypto -lz

Run the VCS commands using the ./mygit executable, followed by the command and any necessary arguments. For example:
./mygit init
./mygit add .
./mygit commit -m "Initial commit"
./mygit log


Implemented Commands

init: Initializes a new MyGit repository by creating a .mygit directory and setting up the necessary structure.
hash-object: Calculates the SHA-1 hash of a file and optionally stores it as a blob object in the repository.
cat-file: Displays the content, type, or size of an object stored in the repository.
write-tree: Creates a tree object that represents the current directory structure.
ls-tree: Lists the contents of a tree object, displaying the mode, object type, SHA hash, and filename.
add: Adds files or directories to the staging area, preparing them for the next commit.
commit: Creates a new commit object, representing a snapshot of the staged changes, and updates the repository's history.
log: Displays the commit history from the latest commit to the oldest.