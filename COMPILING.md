# Building instructions

## What do you want to achieve?

When you want to build the binary yourself, you typically fall into one of the following categories:
* a) You want to develop/build on your local machine, to run it on that machine.
* b) You want to cross-compile on your local machine, to run it on another architecture.
* c) You want to build all the supported formats.

The necessary tasks of the respective categories are explained below. First, some general information.

## General information

### Supported environments

The project is set up to officially support common **desktop** environments:
* Linux (32bit, 64bit)
* MS Windows (32bit, 64bit)
* ~~MacOS~~ (should be, but currently not working after the switch to Apple Silicon)

> The emphasis on *desktop*, above, is deliberate. Only what builds by the automated build jobs
> of this project is supported. If there is a permutation of another system that isn't ensured by a
> build job, it may not work.
> 
> The following describes the ideal working setup. 

### Dependencies

* [CMake](https://cmake.org/) 3.x, with a minimum version according to `CMakeLists.txt`
  > CMake 4 is no longer supporting various constructs found in some library dependencies.
* GNU compiler suite (either that from Linux or MinGW; use WSL or [MSYS2](https://www.msys2.org/))
* Internet connection (for downloading library dependencies for bundled library dependencies)

## How to build?

### You want to develop/build on your local machine and run it on that machine.

#### Simple case
```
# 1) Download library dependencies and prepare build scripts (once, unless project structure is changed):
cmake -B build -S .
# 2) Build binary:
cmake --build build
# 3) Find the executable in the created build/ directory.
```

#### CMake build options

The following CMake options can be used to customize the build process:

| Option              | Possible Values        | Description                                                                                                          |
|---------------------|------------------------|----------------------------------------------------------------------------------------------------------------------|
| `ENABLE_EXAMPLES`   | **OFF** / ON           | Enable example applications (can be broken!)                                                                         |
| `ENABLE_DEBUG_BLIT` | **OFF** / ON           | Enable debugging blitter                                                                                             |
| `ENABLE_OPENGL`     | **ON** / OFF           | Enable OpenGL support for improved graphics rendering                                                                |
| `ENABLE_SDL2`       | **BUNDLED** / ON       | Use system-installed SDL2 or bundled version                                                                         |
| `ENABLE_SOUND`      | **BUNDLED** / ON / OFF | Enable sound support (requires SDL2_mixer); ON uses system library, BUNDLED uses bundled version, OFF disables sound |
| `ENABLE_FLUIDSYNTH` | **BUNDLED** / ON / OFF | Enable FluidSynth MIDI support; ON uses system library, BUNDLED uses bundled version, OFF disables MIDI support      |

To use these options, pass them to CMake with the `-D` flag:

```bash
cmake -B build -S . -DENABLE_OPENGL=OFF -DENABLE_SOUND=OFF
```

> Note that the support for system-provided libraries may be removed at a future point.
> The bundled versions are downloaded and handled by CMake in a defined version. 

### You want to cross-compile on your local machine for a different target.

Cross-compilation is considered to be a build that targets a different architecture than what is default for your machine.
This also includes building a 32-bit variant on your 64-bit machine!

For such cases, use and provide a [CMake toolchain file](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html):
```
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<path-to-your-file>
```
Refer to `cmake/toolchains` subdirectory for the ones this project uses.

### You want to build all the supported formats.

This is essentially the work of the build pipeline. See `.github/workflows/main-build.yml` that combines all the information above.

On a Linux system, or an MS Windows one with WSL2, you can cover at least the Linux and MS Windows binaries,
using GCC and MinGW.
