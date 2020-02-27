Compiling Shockolate
====================

Prerequisites: 
* CMake 3.11
* SDL2 2.0.9
* SDL2_mixer 2.0.4 (optional, for sound)
* FluidSynth (optional)

The following CMake options are supported in the build process:
* `ENABLE_SOUND` - enable sound support (requires SDL2_mixer)
* `ENABLE_FLUIDSYNTH` - enable FluidSynth MIDI support (ON/LITE/OFF, default is embedded LITE)
* `ENABLE_OPENGL` - enable OpenGL support (ON/OFF, default ON)

## Linux

Here example for Ubuntu. Since Shockolate requires decent multimedia libraries, you need add multimedia repository:

```
sudo add-apt-repository -y ppa:savoury1/multimedia
sudo apt-get -q update
sudo apt-get install -y cmake libglu1-mesa-dev libgl1-mesa-dev libsdl2-dev libsdl2-mixer-dev
```

Now you ready for building.

```
mkdir systemshock_build
cd systemshock_build
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_OPENGL=ON -DENABLE_SOUND=ON <path to Shockolate sources>
make -j2
cp -r <path to Shockolate sources>/shaders . ; mkdir res
```

After compilation you'll see systemshock executable in `systemshock_build` directory. 

## macOS

Install dependencies via `brew`:

```
brew install sdl2
brew install sdl2_mixer
```

Now you ready for building.

```
mkdir systemshock_build
cd systemshock_build
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_OPENGL=ON -DENABLE_SOUND=ON <path to Shockolate sources>
make -j2
cp -r <path to Shockolate sources>/shaders . ; mkdir res
```

After compilation you'll see systemshock executable in `systemshock_build` directory. 

## Windows

### 64 bit
Currently on Windows only MINGW environment is supported. We recommended [MSYS2](https://www.msys2.org/) for that.
Install MSYS2 and launch MYS2 MinGW 64-bit shell. Install/update required tools:

```
pacman -Syu --noconfirm     # Close shell and open again
pacman -Syu --noconfirm     # Second run
pacman -Sy --noconfirm mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja
```  

Install dependencies:

```
pacman -Sy --noconfirm mingw-w64-x86_64-glew mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_mixer
```

Now you ready to go. Let's assume that sources resides in `C:\project\systemshock`, then inside MinGW shell it would be
`/c/project/systemshock`

```
mkdir systemshock_build
cd systemshock_build
cmake -G 'Ninja' -DCMAKE_BUILD_TYPE=Release -DPKG_CONFIG_EXECUTABLE=/mingw64/bin/pkg-config.exe -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc.exe -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++.exe -DENABLE_SOUND=ON -DENABLE_OPENGL=ON /c/project/systemshock
cmake --build . -j 2
cp -r /c/project/systemshock/shader . ; mkdir res
```

After compilation you'll see systemshock.exe executable in `systemshock_build` directory.

Importunately, currently MinGW build requires some DLLs from environment, there quick hack for it:

```
cp /mingw64/bin/{libFLAC*,libglib-*,libgmodule-*,libmodplug*,libmpg123*,libportaudio-2,libreadline*,libsndfile*,libogg-*,libtermcap-0,libintl-*,libiconv-*,libopus-0,libpcre-*,libopusfile-0,libvorbis-0,libvorbisenc-2,libvorbisfile-*,libspeex-*}.dll . 
```

### 32 bit

There no big differences comparing to 64 bit. Launch MYS2 MinGW 32-bit shell and paste these commands:

```
pacman -Syu --noconfirm     # Close shell and open again
pacman -Syu --noconfirm     # Second run
pacman -Sy --noconfirm mingw-w64-i686-toolchain mingw-w64-i686-cmake mingw-w64-i686-ninja
pacman -Sy --noconfirm mingw-w64-i686-glew mingw-w64-i686-SDL2 mingw-w64-i686-SDL2_mixer
mkdir systemshock_build
cd systemshock_build
cmake -G 'Ninja' -DCMAKE_BUILD_TYPE=Release -DPKG_CONFIG_EXECUTABLE=/mingw32/bin/pkg-config.exe -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc.exe -DCMAKE_CXX_COMPILER=i686-w64-mingw32-g++.exe -DENABLE_SOUND=ON -DENABLE_OPENGL=ON /c/project/systemshock
cmake --build . -j 2
cp -r /c/project/systemshock/shader . ; mkdir res
```
