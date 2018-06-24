#!/bin/bash

set -e

SDL_version=2.0.8
SDL_mixer_version=2.0.2
CMAKE_version=3.11.3
CMAKE_architecture=win32-x86

if [ -d ./build_ext/ ]; then
	echo A directory named build_ext already exists.
	echo Please remove it if you want to recompile.
	exit
fi

rm -rf CMakeFiles/
rm -rf CMakeCache.txt

mkdir ./build_ext/
cd ./build_ext/

curl -O https://www.libsdl.org/release/SDL2-${SDL_version}.tar.gz
tar xvf SDL2-${SDL_version}.tar.gz
git clone https://github.com/SDL-mirror/SDL_mixer.git

cd ..
'
echo "Next steps: "
echo "1. Build SDL2, SDL2main and SDL_mixer in Visual Studio"
echo "   (you'll need to add SDL2's include and output directories "
echo "   and linker dependencies respectively) "
echo "2. Copy the DLL's in the output directories to this directory"
echo "3. Run CMake-GUI to create a Visual Studio project for Shockolate"
echo "4. Add \$(ProjectDir)\src\GameSrc\Headers\precompiled.h to systemshock and GAME_LIB"
echo "   in Properties -> Configuration Properties -> C/C++ "
echo "   -> Advanced -> Forced Include File "
echo "5. If the linker complains about 'm.lib', remove it from systemshock's dependencies"
echo "   in Properties -> Configuration Properties -> C/C++ -> Linker -> Input"