Shockolate - System Shock, but cross-platform!
============================
Based on the source code for PowerPC released by Night Dive Studios, Incorporated.

GENERAL NOTES
=============

Shockolate is a cross-platform source port of System Shock, using SDL2. This runs well on Linux and Windows right now,
with some missing features that need reviving due to not being included in the source code that was released.

The end goal for this project is something like what Chocolate Doom is for Doom:
an experience that closely mimics the original, but portable and with some quality-of-life improvements,
including an OpenGL renderer and mod support!

Join our Discord to follow along with development: https://discord.gg/m45xPan

![work so far](https://i.imgur.com/kbVWQj4.gif)

Running
=======

## 1) Acquire binary
### From a prebuilt package

Find a list of [downloadable packages](https://github.com/Interrupt/systemshock/releases/) for Linux and Windows.
32- and 64-bit versions are available for Linux and Windows.

### From source code

See [COMPILING.md](COMPILING.md) in the repository.

## 2) Provide resources

### 2a) Game data
Original cd-rom or SS:EE assets in a `res/data` folder need to be next to the executable.
> Floppy disk assets are an older version that we can't load currently.

### 2b) Music sound font (optional)
In the `res` folder, add a `.sf2` sound font file that should be used for playback.

> There is a file on the internet that's close to the Windows default everyone knows: http://rancid.kapsi.fi/windows.sf2
> Please note that this is a file of unknown provenance.

## 3) Execute binary

```
./systemshock
```

### Command line parameters

`-nosplash` Disables the splash screens, causes the game to start straight to the main menu

Modding Support
============
Shockolate supports loading mods and full on fan missions.
Point the executable at a mod file or folder and the game will load it in.
So far mod loading supports additional `.res` and `.dat` files for resources and missions respectively.

Run a fan mission from a folder:
```
./systemshock /Path/To/My/Mission
```

Run a fan mission from specific files:
```
./systemshock my-archive.dat my-strings.res
```

Control modifications
=======

## Movement

Shockolate replaces the original game's movement with WASD controls, and uses `F` as the mouselook toggle hotkey. This differs from the Enhanced Edition's usage of `E` as the mouselook hotkey, but allows us to keep `Q` and `E` available for leaning.

## Additional hotkeys

* `Ctrl+G` cycles between graphics rendering modes
* `Ctrl+F` to enable full screen mode
* `Ctrl+D` to disable full screen mode 
