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
