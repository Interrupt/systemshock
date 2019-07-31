Shockolate - System Shock, but cross platform!
============================
Based on the source code for PowerPC released by Night Dive Studios, Incorporated.

[![Build Status TravisCI](https://travis-ci.org/Interrupt/systemshock.svg?branch=master)](https://travis-ci.org/Interrupt/systemshock) [![Build Status AppVeyor](https://ci.appveyor.com/api/projects/status/5fmcswq8n7ni0o9j/branch/master?svg=true)](https://ci.appveyor.com/project/Interrupt/systemshock)

GENERAL NOTES
=============

Shockolate is a cross platform source port of System Shock, using SDL2. This runs well on OSX, Linux, and Windows right now, with some missing features that need reviving due to not being included in the source code that was released.

The end goal for this project is something like what Chocolate Doom is for Doom: an experience that closely mimics the original, but portable and with some quality of life improvements including an OpenGL renderer and mod support!

Join our Discord to follow along with development: https://discord.gg/m45xPan

![work so far](https://i.imgur.com/kbVWQj4.gif)

Prerequisites
=======
  - Original cd-rom or SS:EE assets in a `res/data` folder next to the executable
    - Floppy disk assets are an older version that we can't load currently


Downloads
=======

We have CI systems in place building [distributable packages](https://github.com/Interrupt/systemshock/releases/) out of tagged commits for Linux, Mac and Windows.

Compiling / Running
============

## Prerequisites
  - SDL2 2.0.8, 32 bit
  - SDL2_mixer 2.0.1, 32 bit
  - OpenGL (glew), 32 bit

## Building SDL
### Linux/Mac
You can use the included `build_deps.sh` shell script to build the required versions of SDL2 / SDL2_mixer. 

### Windows
See [the Windows readme](windows/readme_windows.md).

## Build and run
```
mkdir build
cd build
cmake ..
make systemshock
```

Copy RES folder to build/bin (there should be `RES/DATA/*.RES` and `RES/DATA/ARCHIVE.DAT` files). Now you ready to play SystemShock:

```
cd bin
./systemshock
```

Command line parameters
============

`-nosplash` Disables the splash screens, causes the game to start straight to the main menu

Modding Support
============
Shockolate supports loading mods and full on fan missions. Just point the executable at a mod file or folder and the game will load it in. So far mod loading supports additional `.res` and `.dat` files for resources and missions respectively.

Run a fan mission from a folder:
```
./systemshock /Path/To/My/Mission
```

Run a fan mission from specific files:
```
./systemshock my-archive.dat my-strings.res
```
