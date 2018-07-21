Shockolate - Same great System Shock, new great taste!
https://github.com/Interrupt/systemshock

Based on the source code for PowerPC released by Night Dive Studios, Incorporated.

This is a cross platform source port of the System Shock source code that was released,
using SDL2. This runs well on OSX and Linux right now, with some runtime issues to sort out.

The end goal for this project is something like what Chocolate Doom is for Doom: an
experience that closely mimics the original, but portable and with some quality of life
improvements - and mod support!

# Setup!
This requires the game files in a "res/data" folder next to the exe.

There is basic midi music support if there are exported .mid files in /res/music/ like 'thm0.mid'.
Try this example music pack made of Chicajo MIDIs: https://drive.google.com/open?id=18KhiHpmPHGuTedMCPifnox2DWLd2GnCW

# Mod support!
You can point the game at Fan Mission folders or files via the command line:
systemshock.exe /Path/To/My/Mission
