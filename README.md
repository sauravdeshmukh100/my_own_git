---

# Mini Version Control System (MyGit)

## **Introduction**
This project implements a mini version control system (VCS) called **MyGit**, which mimics the basic functionalities of Git. It allows users to:
- Initialize a repository
- Add files
- Commit changes
- View commit history
- Checkout specific commits

The project provides a simplified yet functional VCS for understanding the core concepts of version control.

---

## **Installation and Usage**

### **Prerequisites**
Ensure you have the following installed on your system:
- A C++ compiler (e.g., `g++` or `clang++`)
- OpenSSL and zlib libraries

### **Setup**
1. Clone the repository and navigate to the project directory.
2. Compile the source files:
   ```bash
   make
   ```
   Or, compile manually:
   ```bash
   g++ main.cpp -o mygit -lssl -lcrypto -lz
   ```

3. Run the commands using the `./mygit` executable.

---

## **Implemented Commands**

### 1. `init`
- **Command:** `./mygit init`
- **Description:** Initializes a new repository by creating a `.mygit` directory with the necessary structure to store objects and metadata.

### 2. `hash-object`
- **Command:** `./mygit hash-object [-w] <filename>`
- **Description:**
  - Computes the SHA-1 hash of a file.
  - With the `-w` flag, stores the file as a blob object in the repository.

### 3. `cat-file`
- **Command:** `./mygit cat-file <flag> <object_sha>`
- **Description:** Displays information about an object in the repository.
  - `-p`: Prints the object's content.
  - `-t`: Displays the object's type.
  - `-s`: Shows the object's size.

### 4. `write-tree`
- **Command:** `./mygit write-tree`
- **Description:** Creates a tree object representing the current directory structure and outputs its SHA-1 hash.

### 5. `ls-tree`
- **Command:** `./mygit ls-tree [--name-only] <tree_sha>`
- **Description:**
  - Lists the contents of a tree object.
  - `--name-only`: Displays only the names of files and directories.

### 6. `add`
- **Command:** `./mygit add <filename>` or `./mygit add .`
- **Description:** Adds files or directories to the staging area, preparing them for the next commit.

### 7. `commit`
- **Command:** `./mygit commit -m "<message>"`
- **Description:** Creates a new commit object representing a snapshot of the staged changes. Updates the repository's history.

### 8. `log`
- **Command:** `./mygit log`
- **Description:** Displays the commit history in reverse chronological order, showing:
  - Commit SHA
  - Parent SHA (if applicable)
  - Commit message
  - Timestamp
  - Committer information

### 9. `checkout`
- **Command:** `./mygit checkout <commit_sha>`
- **Description:**
  - Restores the project state to match the specified commit.
  - Recreates the directory structure and file content associated with the commit.
  - **Note:** This command does not update `HEAD`.

---

## **Assumptions**
- The project assumes that the `.mygit` directory exists after running the `init` command.
- Files are stored as blobs, and directories are represented as tree objects, both compressed for efficiency.
- All commands adhere to Git-like behavior where possible, but certain advanced features (e.g., branches) are not implemented.

---

## **Execution Example**
```bash
./mygit init
./mygit add main.cpp
./mygit commit -m "Initial commit"
./mygit log
./mygit checkout 95d09f2b10159347eece71399a7e2e907ea3df4f
```

---

Feel free to provide feedback or request further customization!