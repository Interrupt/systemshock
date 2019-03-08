#!/usr/bin/env bash

# Setting up 

set -euo pipefail

SDL_version=2.0.9
SDL2_mixer_version=2.0.4

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

function build_fluidsynth {
	git clone https://github.com/Doom64/fluidsynth-lite.git
	pushd fluidsynth-lite

	# force compilation of dynamic library
	sed -i'' -e 's/DLL\"\ off/DLL\"\ on/' CMakeLists.txt
	
	# force 32bit compilation
	sed -i'' -e 's/\${GNUCC_WARNING_FLAGS}/-m32 \${GNUCC_WARNING_FLAGS}/' CMakeLists.txt
	
	# if building fluidsynth fails, move on without it
	set +e
	cmake .
	cmake --build .

	# download a soundfont that's close to the Windows default everyone knows
	curl -o music.sf2 http://rancid.kapsi.fi/windows.sf2
	set -e
	popd
}


# Actual script starts here

if [ -d ./build_ext/ ]; then
	echo A directory named build_ext already exists.
	echo Please remove it if you want to recompile.
	exit
fi

mkdir ./build_ext/
cd ./build_ext/

install_dir=$(pwd)

build_fluidsynth
build_sdl
build_sdl_mixer

cd ..

if [[ -z "$TRAVIS" ]]; then
	mkdir ./lib/
	for i in build_ext/fluidsynth-lite/src/*.so; do [ -f "$i" ] || break; cp $i lib/; done;
	for i in build_ext/fluidsynth-lite/src/*.dylib; do [ -f "$i" ] || break; cp $i lib/; done;

	cp build_ext/built_sdl/lib/libSDL2main.a lib/
	for i in build_ext/built_sdl/lib/libSDL2.so; do [ -f "$i" ] || break; cp $i lib/; done;
	for i in build_ext/built_sdl/lib/libSDL2.dylib; do [ -f "$i" ] || break; cp $i lib/; done;

	for i in build_ext/built_sdl_mixer/lib/libSDL2_mixer.so; do [ -f "$i" ] || break; cp $i lib/; done;
	for i in build_ext/built_sdl_mixer/lib/libSDL2_mixer.dylib; do [ -f "$i" ] || break; cp $i lib/; done;	

	# move the soundfont to the correct place if we successfully built fluidsynth
	for i in build_ext/fluidsynth-lite/*.sf2; do [ -f "$i" ] || break; mv $i res/; done;	

	# clean the build artifacts
	rm -rf build_ext/	
fi
