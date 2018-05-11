System Shock GPL Source Code
============================
Copyright 2015-2018, Night Dive Studios, Incorporated.

GENERAL NOTES
=============

This is my attempt to get System Shock running on modern OSX. Once this runs well here, it should be much easier to port forward to other systems like Linux.

![work so far](https://i.imgur.com/PWdEo9J.gif)

# Working so far:
- Loads original resource files
- Starting up an SDL drawing context, including palette
- Bitmap rendering
- Level loading
- Starting a new game
- Some HUD rendering
- 3D rendering
- AI
- Physics

# Not working:
- Still a few references to Mac libs left
  - Could be made cross platform by removing
- HUD clipping weirdness
  - something in the switch back to the old inventory / HUD assets has things not drawing properly
- Keyboard Input
  - should be switched to SDL2, like rendering and mouse input
- Saving / Loading
  - save and load system needs to be reimplemented
- Main Menu
  - the original main menu should be revived, instead of the old Mac version
- Sound & Music
  - SDL2 should be able to play the VOC and MIDI files used for sfx and music
- Video Files
  - ???

Compiling / Running
============

# Prerequisites
  - SDL2 for 32 bit
  - Original assets in a `res/data` folder next to the executable

# Build and run
```
cmake .
make systemshock
./systemshock
```
