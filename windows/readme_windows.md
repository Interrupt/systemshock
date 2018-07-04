Compiling Shockolate on Windows
============

## Prerequisites
  - Git Bash, 32 bit - **NOTE**: Must be installed in a path without spaces
  - MinGW with G++ installed
  
## Building 
The regular `build_deps.sh` is replaced with a `build_windows.sh` file. When run from the **Git Bash shell**, it will install SDL2 and SDL2_mixer, followed by retrieving CMake if an installation is not found on `PATH`. 

Finally, a `build.bat` to be run in a **Windows shell** is composed for convenience.
