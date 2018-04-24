System Shock GPL Source Code
============================
Copyright 2015-2018, Night Dive Studios, Incorporated.

GENERAL NOTES
=============

This is my attempt to get System Shock compiling on modern OSX with Clang. I'm in hack and slash mode while trying to do this, my goal is to get things compiling first by any means necessary and then go back and start to turn features back on.

# Working so far:
- Loads original resource files
- Starting up an SDL drawing context, including palette
- Bitmap rendering
- Level loading
- Starting a new game
- Some HUD rendering

# Not working:
- Keyboard Input / Mouse Clicks
  - polling should be switched to SDL
- 3d rendering
  - need to implement some integer math functions
- Main Menu
  - can be turned back on when input works
- Sound & Music
- Video Files
- Saving / Loading

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
