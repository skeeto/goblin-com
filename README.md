# Goblin-COM

![](http://i.imgur.com/nz906Wc.gif)

Operate the elite organization, Goblin-COM (Goblin Combat Unit), to
protect your castle island from invading goblins. It's an XCOM-like
squad combat roguelike for 7DRL 2015.

## Download

### Windows

All needed resources are embedded in the executable, so there are no
other files to zip up with it.

* [gcom.exe](https://github.com/skeeto/goblin-com/releases/download/1.0.0/gcom.exe) (64-bit)
* [gcom32.exe](https://github.com/skeeto/goblin-com/releases/download/1.0.0/gcom32.exe) (32-bit)

### Telnet

If you don't want to download anything, you can play over telnet using
any ANSI+UTF-8 terminal emulator. (If it looks bad in PuTTY, [fix its
configuration][putty].)

    telnet gcom.nullprogram.com

Saving is disabled in the telnet version, though.

[putty]: http://thegreyblog.blogspot.com/2009/08/configuring-putty-to-use-utf-8.html

### Other Platforms

For all other platforms it should be easy to build from source.

## Implementation Details

The save file is just a memory dump, so it's not necessarily portable.
However, it's unlikely anyone would want to port a save across
architectures anyway.

No libraries will be used except for small, embeddable ones. I want
this to be a single, simple, tight executable. Modding the game will
require changing the sources, but since it will be trivial to compile
that shouldn't be an issue.

A mini ncurses-like, panel-oriented library has been written
specifically for G-COM as its display driver. It minimizes required
updates to put less load on the terminal emulator and use less
bandwidth in the case of telnet play.

The game is designed from the ground up to support modern (UTF-8) ANSI
terminal emulators, telnet play, and Windows' console in its default
configuration. The display driver is a least common denominator of
this subset (16 colors, limited Unicode support).

### Unicode

G-COM's Unicode support is only partial, just enough to display some
interesting, pretty characters in the game. Full support would require
including the big Unicode tables. The limitations:

* Can't input Unicode. Just ASCII.
* Can't handle combining characters. It thinks they take up space, so
  stick with precomposed characters.
* Similarly, no support glyphs that are more or less than one space
  wide.
* Can't handle characters in the astral planes. This is emphasized by
  the `uint16_t` type for characters. Fonts for the astral planes are
  rarely ever available, so those characters won't display properly
  anyway even in modern terminals. I also don't want to bother with
  surrogate pair encodings for the sake of Windows' outdated UTF-16
  support. This is kind of a shame because some of the best characters
  for games (trees, castles, mountains, etc.) are in the astral
  planes.

Unicode support in Windows' console is total rubbish, probably from
not having been updated in 20 something years. This limits the
character selection to essentially the old IBM code pages (but using
the newer Unicode codepoints). Fortunately there are still enough
characters available to be interesting.
