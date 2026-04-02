# Doom Legacy 2.0 — The Legacy of Doom

Home page: http://doomlegacy.sourceforge.net/

## Contents

1. [Introduction](#introduction)
2. [The basics](#the-basics)
   1. [Installation](#installation)
   2. [Launcher](#launcher)
   3. [Command-line options](#command-line-options)
3. [The game](#the-game)
   1. [Advanced features](#advanced-features)
4. [Editing](#editing)
   1. [Legacy editing references](#legacy-editing-references)
   2. [Standard references](#standard-references)
5. [Development](#development)
6. [Copyright info and acknowledgments](#copyright-info-and-acknowledgments)

Old stuff:
- [faq.html](faq.html) Frequently Asked Questions answered
- [history.html](history.html) History of Doom Legacy

---

## Introduction

Greetings to all the Doom fans of the world, and welcome to the latest release of Doom Legacy!
This version introduces Hexen compatibility, new netcode, better portability and much more.

Since Doom was out we have been fans of deathmatching. Don't ask us why, but we liked it more than Duke Nukem, Quake or other clones. We simply enjoy the atmosphere, look and feel of Doom. We always dreamed of Doom sources being released, and of all the features that we'd like to add. Then, in late 1997, id Software made public the full sources of the greatest game on Earth, Doom. A great number of open source projects have started around the sources since, and Doom Legacy is competing to be the best of the choices!

Doom Legacy can run Doom, Boom, Heretic and Hexen maps. For each game, you will need the corresponding IWAD file, which contains all the graphics, sounds and music in the game. For example, if you want to play Doom II maps, you'll need the file "doom2.wad". Doom Legacy is free software, meaning that you can copy and distribute it without charge. Note that the original IWAD data files of Doom, Heretic and Hexen (doom.wad, doom2.wad, heretic.wad, hexen.wad) are still copyrighted products and available commercially. There are also some free alternative IWADs available on the Internet.

We hope that you enjoy this FPS classic. Have much fun!

### Need more info?

If you have some questions or problems concerning Doom Legacy, please **first read this manual**. If you find no answer, try reading the **FAQ** on our [website](http://legacy.newdoom.com/). If still no luck, try asking your question on the Doom Legacy forum at [New Doom](http://forums.newdoom.com/).

---

## The basics

### Installation

Installing Doom Legacy is extremely simple, just follow these steps:

- Unpack the archive
- Start the Launcher
- Fill in required information about WAD locations and hardware options
- Launch the game!

If you want, you can set the following environment variables to have additional control over Legacy behaviour. None of them are required.

- The environment variable **DOOMWADDIR** defines the path where the wad files are located. This way you can easily share your wad files with different programs. By default, Legacy looks for wad files in the current directory.
- The variable **USER**, containing your username, is used as the default value for your player name in multiplayer games.
- Your home directory, defined by the variable **HOME**, will be the place where Legacy stores your config and savegame files. If this variable is not defined, they will be stored in the current directory.

### Launcher

The Launcher is a separate program, included in the Legacy distribution, which provides the easiest way to start a Doom Legacy game. Using it, you don't have to remember the command-line syntax. Just use the mouse to select the game options you want and the Launcher will start Legacy for you.

### Command-line options

Here are the command-line options Legacy recognizes:

> FIXME: console commands cd, gr_stats, nodownload consvar
> FIXME: video mode lists, window caption, cv_fullscreen..

| Option | Description |
|---|---|
| **General options** | |
| `+<console expression>` | You can enter any number of [console expressions](console.md) at the command line. They will be executed right after the configfile, but before autoexec.cfg. |
| `-iwad <filename>` | Set the main IWAD file to use. This determines the game mode. |
| `-file <filename1> [<filename2>...]` | Load one or more additional data files (wad, pak, lmp, deh...). You can also enter a directory name here by ending it with a slash /. |
| `-home <dirname>` | Set your home directory location, where your configfiles and savegames are kept. |
| `-config <filename>` | Set the configfile to use. Default: config.cfg. |
| `-mb <number>` | Give the amount of heap memory (in MB) Legacy should use. |
| `-opengl` | Start the game in OpenGL mode. Default: use the software renderer. |
| `-devparm` | "Development mode". Prints out some extra information mainly useful for developers. |
| `-noversioncheck` | Do not check that legacy.wad version matches the executable version. |
| `-skill <1-5>` | Select the skill level, start game immediately. |
| `-episode <number>` | Select the episode, start game immediately. |
| **Sound and music** | |
| `-nosound` | Disable sound effects. |
| `-nomusic` | Disable music. |
| `-nocd` | Disable CD music. |
| `-precachesound` | |
| **Networking** | |
| `-dedicated` | Spawn a dedicated server. |
| `-server` | Spawn a server. |
| `-connect [<IP> \| <address>]` | Connect to a server at the specified address. If no address is given, search for servers in LAN. |
| `-port` | Set the UDP port to use. |
| `-ipx` | Use IPX network protocol instead of IP. |
| `-nodownload` | Do not download any files from servers. |

---

## The game

TODO: explain menus, controls, multiplayer stuff, splitscreen, bots.

### Advanced features

Doom Legacy supports most of the cheats in the original games. Additionally, we have added the cheats **idfly** and **idcd**. With idfly, you can fly by pressing and holding the jump button. idcd allows you to quickly change the cd music track currently playing.

- [console.md](console.md) Console commands explained
- [skins.html](skins.html) Legacy skin specifications

---

## Editing

### Legacy editing references

Powerful editing features have long been one of Legacy's strongest points. As of version 2.0, the Legacy game engine has been rewritten in C++. During the rewrite, loads of new features were added to the game.

- Summary of Doom Legacy [features](features.md).
- Doom Legacy [Map Author's Reference](editing.md).
- [FraggleScript basics](fsbasic.md) and [FraggleScript function reference](fsfuncs.html).

### Standard references

These documents are the current standard references to the inner workings of Doom, Boom, Heretic and Hexen. Although many of the limitations of the original games have been lifted and new features have been added, they are still very useful to a Legacy map editor.

- Matt Fell's [Unofficial Doom specs v1.666](http://www.gamers.org/dEngine/doom/spec/uds.1666.txt)
- Team TNT's [Boom reference v1.3](boomref.html)
- Dr Sleep's [Unofficial Heretic specs v1.0](http://www.hut.fi/u/vberghol/doom/hereticspec10.txt)
- Ben Morris's [Hexen specs v0.9](http://www.hut.fi/u/vberghol/doom/hexenspec09.txt)

---

## Development

Doom Legacy is free software, distributed under GPL, the GNU General Public License. The source can be found [here](http://sourceforge.net/projects/doomlegacy/).

- Here are the [compiling instructions](compiling.md).
- What you should know about the [source](source.md).
- Game [subsystem definitions](elements.md) and progress indicators.
- [Todo](TODO.md) list.
- [Bugs](BUGS.md) list.

Also, the source itself has been documented using [Doxygen](html/index.html). You'll have to compile the documentation first, though.

---

## Copyright info and acknowledgments

Original game & sources by id Software
Heretic and Hexen by Raven Software

Additions:
© 1998 by: Fabrice Denis & Boris Pereira
© 1999 by: Fabrice Denis, Boris Pereira & Thierry Van Elsuwe
© 2000 by: Boris Pereira & Thierry Van Elsuwe
© 2001-2005 by: Doom Legacy Team

- Thanks to id Software of course, for creating the greatest game of all time!
- To Raven Software, for Heretic and Hexen!
- Chi Hoang for DosDoom which got us started.
- Bell Kin for porting DooM Legacy to Linux.
- Stephane Dierickx for the two pictures in the launcher and the help screen of Legacy.
- Sebastien Bacquet for Qmus2mid.
- Simon Howard for FraggleScript.
- TeamTNT for Boom.
- Steven McGranahan and Robert Bäuml for their large contributions.
- To all of you who send us ideas and bug reports, thank you!
