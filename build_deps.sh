#!/bin/bash
set -e

SDL_version=2.0.8
SDL_mixer_version=2.0.2

if [ -d ./built_ext/ ]; then
	echo A directory named build_ext already exists.
	echo Please remove it if you want to recompile.
	exit
fi

mkdir ./build_ext/
cd ./build_ext/

install_dir=$(pwd)

function build_sdl {
	curl -O https://www.libsdl.org/release/SDL2-${SDL_version}.tar.gz
	tar xvf SDL2-${SDL_version}.tar.gz
	pushd SDL2-${SDL_version}

	./configure "CFLAGS=-m32" "CXXFLAGS=-m32" "LDFLAGS=-m32"
	make
	make install

	popd
}

function build_sdl_mixer {
	git clone https://github.com/SDL-mirror/SDL_mixer.git
	#tar xvf SDL2_mixer-${SDL_mixer_version}.tar.gz
	#pushd SDL2_mixer-${SDL_mixer_version}
	pushd SDL_mixer

	./configure "CFLAGS=-m32" "CXXFLAGS=-m32" "LDFLAGS=-m32"
	make
	make install

	popd
}

build_sdl
build_sdl_mixer
