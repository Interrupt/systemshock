@REM Initial build.bat, no changes needed if you already have CMake in PATH

cmake -G "MinGW Makefiles" .
mingw32-make systemshock
