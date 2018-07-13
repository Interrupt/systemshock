#!/usr/bin/env bash

set -euo pipefail

SDL_version=2.0.8

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
	git clone --depth 10 https://github.com/SDL-mirror/SDL_mixer.git
	pushd SDL_mixer
	git checkout -b 23-05-2018 7cad09d

	export SDL2_CONFIG="/usr/local/bin/sdl2-config"
	./configure "CFLAGS=-m32" "CXXFLAGS=-m32" "LDFLAGS=-m32"
	make
	make install

	popd
}

build_sdl
build_sdl_mixer

# Clean up download artifacts
rm SDL2-${SDL_version}.tar.gz
rm -fr SDL2-${SDL_version}
rm -fr SDL_mixer
