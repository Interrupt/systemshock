System Shock GPL Source Code
============================
Copyright 2015-2018, Night Dive Studios, Incorporated.

GENERAL NOTES
=============

This is my attempt to get System Shock compiling on modern OSX with Clang. I'm in hack and slash mode while trying to do this, my goal is to get things compiling first by any means necessary and then go back and start to turn features back on.

Compiling / Installing
============

```
cmake .
make systemshock
```

So far I've been able to revive a bit of the renderer via SDL, that can be tested with:

```
make TestSimpleMain
./TestSimpleMain
```
