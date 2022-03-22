#!/bin/bash
set -e

SDL_version=2.0.9
SDL2_mixer_version=2.0.4

if [ -d ./build_ext/ ]; then
	echo A directory named build_ext already exists.
	echo Please remove it if you want to recompile.
	exit
fi

if [ ! -d ./res/ ]; then
	mkdir ./res/
fi

mkdir ./build_ext/
cd ./build_ext/

install_dir=$(pwd)

function build_sdl {
	curl -O https://www.libsdl.org/release/SDL2-${SDL_version}.tar.gz
	tar xvf SDL2-${SDL_version}.tar.gz
	pushd SDL2-${SDL_version}

	./configure --prefix=${install_dir}/built_sdl
	make
	make install

	popd
}

function build_sdl_mixer {
	curl -O https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-${SDL2_mixer_version}.tar.gz
	tar xvf SDL2_mixer-${SDL2_mixer_version}.tar.gz
	pushd SDL2_mixer-${SDL2_mixer_version}

	export SDL2_CONFIG="${install_dir}/built_sdl/bin/sdl2-config"
	./configure --prefix=${install_dir}/built_sdl_mixer
	make
	make install

	popd
}

function build_fluidsynth {
	git clone https://github.com/EtherTyper/fluidsynth-lite.git
	pushd fluidsynth-lite
	sed -i 's/DLL"\ off/DLL"\ on/' CMakeLists.txt
	# if building fluidsynth fails, move on without it
	set +e

	#export CFLAGS="-m32"
	#export CXXFLAGS="-m32"

	cmake .
	cmake --build .

	# download a soundfont that's close to the Windows default everyone knows
	curl -o music.sf2 http://rancid.kapsi.fi/windows.sf2
	set -e
	popd
}


build_sdl
build_sdl_mixer
build_fluidsynth

cd ..
mv build_ext/fluidsynth-lite/*.sf2 ./res
