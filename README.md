Shockolate - System Shock, but cross platform!
============================
Based on the source code for PowerPC released by Night Dive Studios, Incorporated.

GENERAL NOTES
=============

A cross platform source port of the System Shock source code that was released, using SDL2. This runs well on OSX and Linux right now, with some runtime issues to sort out.

The end goal for this project is something like what Chocolate Doom is for Doom: an experience that closely mimics the original, but portable and with some quality of life improvements - and mod support!

![work so far](https://i.imgur.com/kbVWQj4.gif)

# Working so far:
- Gameplay!
- Mouse and keyboard input
- Software 3D / 2D rendering
- Physics
- Saving and loading
- Sound effects
- Basic midi music, if available
- Mod support!

# Not working:
- Music
  - SDL_Mixer can't play the multi track XMI midi files, need to find another solution for those
  - There is basic midi music support if there are .mid files in `/res/data/music` like `thm0.mid`
- Main Menu
  - the original main menu should be revived, instead of the Mac version
- Video Files / Audiologs
  - Need to revive the old movie rendering code in AFile

Compiling / Running
============

# Prerequisites
  - SDL2, 32 bit
  - SDL2_mixer, 32 bit
  - Original assets in a `res/data` folder next to the executable
  
# Building SDL
You can use the included `build_deps.sh` shell script to build the required versions of SDL2 / SDL2_mixer. VOC support was broken until recently in SDL_mixer, so for sound effects to work you'll probably need to build it from the latest sources like that script does.

# Build and run
```
cmake .
make systemshock
./systemshock
```
