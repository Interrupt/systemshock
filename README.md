Shockolate - System Shock, but cross platform!
============================
Based on the source code for PowerPC released by Night Dive Studios, Incorporated.

[![Build Status](https://travis-ci.org/Interrupt/systemshock.svg?branch=master)](https://travis-ci.org/Interrupt/systemshock)

GENERAL NOTES
=============

Shockolate is a cross platform source port of System Shock, using SDL2. This runs well on OSX, Linux, and Windows right now, with some missing features that need reviving due to not being included in the source code that was released.

The end goal for this project is something like what Chocolate Doom is for Doom: an experience that closely mimics the original, but portable and with some quality of life improvements - and mod support!

![work so far](https://i.imgur.com/kbVWQj4.gif)

# Missing Features:
- Music
  - SDL_Mixer can't play the multi track XMI midi files, need to find another solution for those
  - There is basic midi music support if there are .mid files in `/res/music/` like `thm0.mid`.
    Try this [example music pack made of Chicajo midi's](https://drive.google.com/open?id=18KhiHpmPHGuTedMCPifnox2DWLd2GnCW)
- Video Files / Audiologs
  - Need to revive the old movie rendering code in AFile

Downloads
=======

### Linux/Mac
For now, compiling the source yourself is the way to go. See the Compiling / Running section below.

### Windows
We're making intermittent [releases](https://github.com/Interrupt/systemshock/releases) while setting up a CI system to automatically produce up-to-date ones.

Compiling / Running
============

## Prerequisites
  - SDL2, 32 bit
  - SDL2_mixer, 32 bit
  - Original cd-rom or SS:EE assets in a `res/data` folder next to the executable
    - Floppy disk assets are an older version that we can't load currently

## Building SDL
### Linux/Mac
You can use the included `build_deps.sh` shell script to build the required versions of SDL2 / SDL2_mixer. VOC support was broken until recently in SDL_mixer, so for sound effects to work you'll probably need to build it from the latest sources like that script does.

### Windows
See [the Windows readme](windows/readme_windows.md).

## Build and run
```
cmake .
make systemshock
./systemshock
```

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
