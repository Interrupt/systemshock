#!/bin/bash
set -e

SDL_version=2.0.10
SDL2_mixer_version=2.0.4
GLEW_version=2.1.0
CMAKE_target=Unix\ Makefiles

# Removing the mwindows linker option lets us get console output
function remove_mwindows {
	sed -i -e "s/ \-mwindows//g" Makefile
}

function build_sdl {
	curl -O https://www.libsdl.org/release/SDL2-${SDL_version}.tar.gz
	tar xvf SDL2-${SDL_version}.tar.gz
	pushd SDL2-${SDL_version}

	./configure --host=x86_64-w64-mingw32 --prefix=${install_dir}/built_sdl
	remove_mwindows
	make
	make install

	popd
}

function build_sdl_mixer {
	curl -O https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-${SDL2_mixer_version}.tar.gz
	# Do not extract the Xcode subdirectory because it contains symlinks.
	# They cannot be extracted on Windows because the files in the archive are in the wrong order so
	# the target of the link cannot be found at the time of the extraction.
	tar xf SDL2_mixer-${SDL2_mixer_version}.tar.gz --exclude=Xcode
	pushd SDL2_mixer-${SDL2_mixer_version}

	./configure --host=x86_64-w64-mingw32  --disable-sdltest --with-sdl-prefix=${install_dir}/built_sdl --prefix=${install_dir}/built_sdl_mixer 
	
	remove_mwindows
	make
	make install

	popd
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
	git clone https://github.com/EtherTyper/fluidsynth-lite.git
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

## Actual building starts here

if [ -d ./build_ext/ ]; then
	echo A directory named build_ext already exists.
	echo Please remove it if you want to recompile.
	exit
fi

if ! [ -x "$(command -v cmake)" ]; then
	echo CMake is needed to install Shockolate. 
	echo Please download CMake from https://cmake.org/download/,
	echo install it and try again in a new Git Bash window.
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
	cmake -G \"${CMAKE_target}\" .
	mingw32-make systemshock" >build.bat
else
	echo "Appveyor"
	echo "cmake -G \"${CMAKE_target}\" . 
	make systemshock" >build.bat
fi

echo "Our work here is done. Run BUILD.BAT in a Windows shell to build the actual source."
