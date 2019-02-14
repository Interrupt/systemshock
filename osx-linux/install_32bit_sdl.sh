#!/usr/bin/env bash

set -euo pipefail

SDL_version=2.0.9
SDL2_mixer_version=2.0.4

if [ -d ./build_ext/ ]; then
	echo A directory named build_ext already exists.
	echo Please remove it if you want to recompile.
	exit
fi

mkdir ./build_ext/
cd ./build_ext/

install_dir=$(pwd)

function build_sdl {
	curl -O https://www.libsdl.org/release/SDL2-${SDL_version}.tar.gz
	tar xf SDL2-${SDL_version}.tar.gz
	pushd SDL2-${SDL_version}

	./configure "CFLAGS=-m32" "CXXFLAGS=-m32" "LDFLAGS=-m32" --prefix=${install_dir}/built_sdl
	make -j2
	make install

	popd
}

function build_sdl_mixer {
	curl -O https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-${SDL2_mixer_version}.tar.gz
	tar xf SDL2_mixer-${SDL2_mixer_version}.tar.gz
	pushd SDL2_mixer-${SDL2_mixer_version}
	
	export SDL2_CONFIG="${install_dir}/built_sdl/bin/sdl2-config"
	./configure "CFLAGS=-m32" "CXXFLAGS=-m32" "LDFLAGS=-m32" --prefix=${install_dir}/built_sdl_mixer
	make -j2
	make install

	popd
}

build_sdl
build_sdl_mixer

cd ..

# Create lib directory and clean up download artifacts if building locally
if [[ -z "$TRAVIS" ]]; then
	rm -rf build_ext/
else
	mkdir ./lib/
	cp build_ext/built_sdl/lib/libSDL2main.a lib/
	if [[ "$TRAVIS_OS_NAME" = "linux" ]]
		cp build_ext/built_sdl/lib/libSDL2.so lib/
		cp build_ext/built_sdl_mixer/lib/libSDL2_mixer.so lib/
	else
		cp build_ext/built_sdl/lib/libSDL2.dylib lib/
		cp build_ext/built_sdl_mixer/lib/libSDL2_mixer.dylib lib/
	fi
	ls -la lib/
fi
