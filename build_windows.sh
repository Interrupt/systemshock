#!/bin/bash
set -e

SDL_version=2.0.8
SDL_mixer_version=2.0.2
CMAKE_version=3.11.3
#CMAKE_architecture=win64-x64
CMAKE_architecture=win32-x86

# Removing the mwindows linker option lets us get console output
function remove_mwindows {
	sed -i -e "s/ \-mwindows//g" Makefile
}

function build_sdl {
	curl -O https://www.libsdl.org/release/SDL2-${SDL_version}.tar.gz
	tar xvf SDL2-${SDL_version}.tar.gz
	pushd SDL2-${SDL_version}

	./configure "CFLAGS=-m32" "CXXFLAGS=-m32" --host=i686-w64-mingw32 --prefix=${install_dir}/built_sdl
	remove_mwindows
	make
	make install

	popd
}

function build_sdl_mixer {
	git clone https://github.com/SDL-mirror/SDL_mixer.git
	pushd SDL_mixer

	./configure "CFLAGS=-m32" "CXXFLAGS=-m32" --host=i686-w64-mingw32 --disable-sdltest --with-sdl-prefix=${install_dir}/built_sdl --prefix=${install_dir}/built_sdl_mixer 
	
	remove_mwindows
	make
	make install

	popd
}

function build_glew {
	curl -O https://netcologne.dl.sourceforge.net/project/glew/glew/2.1.0/glew-2.1.0.tgz
	tar xvf glew-2.1.0.tgz
	pushd glew-2.1.0
	mingw32-make glew.lib
	popd
}

function get_cmake {
	curl -O https://cmake.org/files/v3.11/cmake-${CMAKE_version}-${CMAKE_architecture}.zip
	unzip cmake-${CMAKE_version}-${CMAKE_architecture}.zip
	pushd cmake-${CMAKE_version}-${CMAKE_architecture}/bin
	CMAKE_ROOT=`pwd -W`/
	popd
}

## Actual building starts here

if [ -d ./build_ext/ ]; then
	echo A directory named build_ext already exists.
	echo Please remove it if you want to recompile.
	exit
fi

rm -rf CMakeFiles/
rm -rf CMakeCache.txt

cp windows/make.exe /usr/bin/
mkdir ./build_ext/
cd ./build_ext/
install_dir=`pwd -W`

build_sdl
build_sdl_mixer
build_glew

if ! [ -x "$(command -v cmake)" ]; then
	echo "Getting CMake"
	get_cmake
fi

# Back to the root directory, copy SDL DLL files for the executable
cd ..
cp build_ext/built_sdl/bin/SDL*.dll .
cp build_ext/built_sdl_mixer/bin/SDL*.dll .
cp build_ext/glew-2.1.0/lib/*.dll .

# Set up build.bat
if [[ -z "${APPVEYOR}" ]]; then
	echo "Normal build"
	echo "@echo off
	set PATH=%PATH%;${CMAKE_ROOT}
	cmake -G \"MinGW Makefiles\" .
	mingw32-make systemshock" >build.bat
else
	echo "Appveyor"
	echo "cmake -G \"Unix Makefiles\" . 
	make systemshock" >build.bat
fi

echo "Our work here is done. Run BUILD.BAT in a Windows shell to build the actual source."
