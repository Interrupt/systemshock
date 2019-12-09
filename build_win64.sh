#!/bin/bash
set -e

SDL_version=2.0.10
SDL2_mixer_version=2.0.4
GLEW_version=2.1.0
CMAKE_version=3.11.3
CMAKE_architecture=win64-x64
#CMAKE_architecture=win32-x86
CMAKE_target=Unix\ Makefiles

# Removing the mwindows linker option lets us get console output
function remove_mwindows {
	sed -i -e "s/ \-mwindows//g" Makefile
}

function build_sdl {
	curl -O https://www.libsdl.org/release/SDL2-devel-${SDL_version}-mingw.tar.gz
	tar xvf SDL2-devel-${SDL_version}-mingw.tar.gz
	cp -r SDL2-${SDL_version}/x86_64-w64-mingw32/ built_sdl/
}

function build_sdl_mixer {
	curl -O https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-devel-${SDL2_mixer_version}-mingw.tar.gz
	tar xf SDL2_mixer-devel-${SDL2_mixer_version}-mingw.tar.gz --exclude=Xcode
	cp -r SDL2_mixer-${SDL2_mixer_version}/x86_64-w64-mingw32/ built_sdl_mixer/
}

function build_glew {
	curl -O https://netcologne.dl.sourceforge.net/project/glew/glew/${GLEW_version}/glew-${GLEW_version}.tgz
	tar xvf glew-${GLEW_version}.tgz
	mv glew-${GLEW_version}/ built_glew/
	pushd built_glew
	mingw32-make glew.lib
	popd
}

function build_fluidsynth {
	git clone https://github.com/Doom64/fluidsynth-lite.git
	pushd fluidsynth-lite
	sed -i 's/DLL"\ off/DLL"\ on/' CMakeLists.txt
	# if building fluidsynth fails, move on without it
	set +e
	cmake -G "${CMAKE_target}" .
	cmake --build .

	# download a soundfont that's close to the Windows default everyone knows
	curl -o music.sf2 http://rancid.kapsi.fi/windows.sf2
	set -e
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

if [ ! -d ./res/ ]; then
mkdir ./res/
fi

mkdir ./build_ext/
cd ./build_ext/
install_dir=`pwd -W`

if ! [ -x "$(command -v cmake)" ]; then
	echo "Getting CMake"
	get_cmake
fi


build_sdl
build_sdl_mixer
build_glew
build_fluidsynth


# Back to the root directory, copy required DLL files for the executable
cd ..
cp build_ext/built_sdl/bin/SDL*.dll .
cp build_ext/built_sdl_mixer/bin/SDL*.dll .
cp build_ext/built_glew/lib/*.dll .
cp build_ext/fluidsynth-lite/src/*.dll .

# move the soundfont to the correct place if we successfully built fluidsynth
mv build_ext/fluidsynth-lite/*.sf2 ./res

# Set up build.bat
if [[ -z "${APPVEYOR}" ]]; then
	echo "Normal build"
	echo "@echo off
	set PATH=%PATH%;${CMAKE_ROOT}
	cmake -G \"${CMAKE_target}\" .
	mingw32-make systemshock" >build.bat
else
	echo "Appveyor"
	echo "cmake -G \"${CMAKE_target}\" . 
	make systemshock" >build.bat
fi

echo "Our work here is done. Run BUILD.BAT in a Windows shell to build the actual source."
