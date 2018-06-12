Setting up development on Windows
============

## Prerequisites
  - Git Bash 
  - MinGW with G++ installed
  
## Building dev requirements
The regular `build_deps.sh` is replaced with a `build_windows.sh` file. When run from the Git Bash shell, it will fetch SDL2 and SDL2_mixer like the regular installer does, followed by retrieving CMake if an installation is not found on `PATH`. 

Finally, a `build.bat` to be run in a Windows shell is provided for convenience.
