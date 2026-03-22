# Joint MAPINFO Standard

### Version 0.5 (draft), released 2006-07-20.

## Introduction

MAPINFO is a plaintext ASCII lump which is used for defining

- global map properties,
- map clusters and hubs,
- game entrypoints.

Normally there's just one MAPINFO lump in a WAD file, named MAPINFO.

## Basics

The MAPINFO lump consists of any number of map, cluster and episode definitions. Each definition begins with a specific keyword and ends when a new definition begins. The body of a definition consists of lines, each containing a single keyword, possibly followed by data items separated by whitespace and ending in a newline (ASCII 0x0A). Carriage returns (ASCII 0x0D), spaces and tabs are treated as whitespace.

- There are three types of data items: **integers**, **floats** and **strings**.
- All data items (not keywords!) may (but are not required to) be quoted using double quotes. Quoted string-type data is to be interpreted as a C string literal. Most importantly, `\n` is the escape sequence for a newline and `\"` the escape sequence for a double quote. An empty string is denoted as `""`.
- In this specification, required data items are denoted in angle brackets, optional data items in brackets:
  `keyword <int required> [int optional1] [int optional2]`
- In case of multiple definitions of any property, later definitions always override the earlier definitions.
- Comments start with a semicolon as the first character of the line and end after the next newline.
- Whenever textures (graphics) are referred to by name, they should be found using the Graphic lookup order as defined in the JDS [Joint Texture Standard](http://glbsp.sourceforge.net/JointTextureSpec.txt), unless specified otherwise.

## Map definitions

A map definition begins with the keyword `map`, followed by the map separator lumpname (e.g. MAP04, E3M1 etc.), a nice long name for the map (e.g. "The Nuclear Plant"), and the level number (a unique positive number used by the exit teleporters destined for this map):

```
map <str lumpname> <str nicename> [int levelnumber]
```

If (and only if) the map lumpname is of the form MAPxx, the levelnumber may be omitted (and is taken to be xx). You can also use the keyword `defaultmap` to start the definition for the default map properties:

```
defaultmap
```

The keywords for defining map properties are:

| Keyword | Default value(s) | Description |
|---|---|---|
| **Basic map properties** | | |
| `cluster <int number>` | 0 | Number of the cluster in which the map belongs. |
| `nextlevel <int next> [int secretnext]` | -1, -1 | Defines level numbers of the exit destinations for Doom/Heretic-style exits. A value of -1 means that the game ends when the exit is activated. |
| `next <str maplumpname>` | "" | Alternative way of defining the Doom/Heretic exit destination. |
| `secretnext <str maplumpname>` | "" | Alternative way of defining the Doom/Heretic secret exit destination. |
| `author <str name>` | "" | Map author's name. |
| `version <str versionstring>` | "" | Version string for the map. |
| `description <str descr>` | "" | Map description. |
| `music <str lumpname>` | "" | Music to be played in the map. |
| `par <int seconds>` | 0 | Map partime in seconds. |
| `gravity <float acceleration>` | 1.0 | Map default gravity. Can be overridden by specific constructs inside the map. |
| **Intermission-related options** | | |
| `picname <str texturename>` | "" | The graphic to use instead of the map nice name in the intermission screens etc. |
| `interpic <str texturename>` | "" | Intermission background picture to show after the map is finished. |
| `intermusic <str lumpname>` | "" | Music for the intermission. |
| **Additional stuff** | | |
| `sky1 <str texturename> <float scrollspeed>` | "", 0.0 | Defines the sky texture and the speed it scrolls to the left. Use the Wall lookup order. |
| `sky2 <str texturename> <float scrollspeed>` | "", 0.0 | Defines the secondary sky texture. Use the Wall lookup order. |
| `doublesky` | | Specifies that both sky textures are drawn, with sky1 in front of sky2. Sky2 will show through wherever sky1 is color 0. |
| `lightning` | | Turns on the Hexen lightning effect in the map. |

## Cluster definitions

Clusters are used for grouping maps together. Each map belongs to a single cluster. If a map has no cluster keyword, it is assigned to special cluster 0, which is automatically generated. A cluster definition begins with the keyword `clusterdef`, followed by a positive cluster number and a nice long name for the cluster (e.g. "Knee-deep in the Dead"):

```
clusterdef <int number> [str nicename]
```

The keywords for defining cluster properties are:

| Keyword | Default value(s) | Description |
|---|---|---|
| `entertext <str text>` | "" | |
| `exittext <str text>` | "" | |
| `music <str lumpname>` | "" | Music to be played during the enter- and exittext. |
| `flat <str texturename>` | "" | Background picture for the texts (will be tiled if smaller than 320×200). Use the Flat lookup order. |
| `hub` | | Indicates that the cluster is a hub. Inside a hub, the states of all maps are saved when they are exited, and loaded when they are re-entered. |
| `finale <str finalescript>` | "" | Which finale to show if the game ends in this cluster? Some script names are reserved, see the following table. |

This document does not attempt to define the finale scripts in any way. However, the following reserved finale script names should always produce the expected finales:

| Script name | Description |
|---|---|
| Doom1 | Doom 1 finales (episode dependent) |
| Doom2 | Doom 2 cast presentation |
| Heretic | Heretic finales (episode dependent) |
| Hexen | Hexen chessboard scene |

## Episode definitions

Episodes are used for defining entry points to the game, visible in the game menu. The episodes should appear in the game menu in the same order they are defined. An episode definition begins with the keyword `episode`, followed by the lumpname of the initial map, the name of the episode, and possibly the in-map entrypoint (Hexen only):

```
episode <str maplump> <str nicename> [int entrypoint]
```

The keywords for defining episode properties are:

| Keyword | Default value(s) | Description |
|---|---|---|
| `picname <str texturename>` | "" | The graphic to use instead of the nice name in the menu. |

## Example

Here is a self-contained example MAPINFO lump, which defines a game consisting of two clusters, six maps and two entrypoints:

```
; the normal entrypoint
episode MAP01 "The Mare Imbrium Fiasco"

; an alternative entrypoint for beginners
episode TRAINING "Basic Training"


; First chapter
clusterdef 1 "The Mare Imbrium Base"
hub
flat SLIME16
music D_ULTIMA
exittext "Un-frakking-believable! Another illegal UAC experiment gone bad, another dimension rift to fix.\nAnd guess who's the only one alive to do the job?"

defaultmap
cluster 1
author "Smite-meister"
sky1 SKY1 0
interpic INTERPIC
intermusic D_RUNNIN

map MAP01 "Mission control"
version "1.0"
description "Lots of SP scripting, not suitable for DM."
music D_COUNTD
par 50
nextlevel 2 3

map MAP02 "Power core"
version "1.2"
description "Great DM map for 4-8 players!"
music D_ROMERO
par 80
nextlevel 3 1

map MAP03 "The Bianchini anomaly"
version "1.1"
description "Works also as a three team CTF map. Watch out for snipers."
music "music/anomaly.ogg"
par 100
gravity 0.2
nextlevel 10

map TRAINING "The Marine Barracs" 99
version "0.9"
description "Training map, also good for 4 player DM."
music "music/barracks.ogg"
par 30
nextlevel 1


; Second chapter
clusterdef 2 "Irem, the City of Pillars"
flat FLOOR25
music MUS_CPTD
exittext "So much for the bat-demons. You just knew that someday your BA in Sumerian Linguistics would come in handy."
finale "my_awesome_finale"

defaultmap
cluster 2
sky1 SKY2 50
interpic DESERT
intermusic MUS_INTR

map E1M2 "The Catacombs" 10
author "unknown"
description "Uncredited PD map I found somewhere."
music MUS_E1M6
par 130
nextlevel 11

map E8M5 "Temple of Marduk" 11
author "me"
version "0.97 alpha5"
description "Huge SP map with a large central area perfect for team deathmatch."
music MUS_E1M2
par 210
nextlevel -1
;episode ends here
```

## Comments

- Should the `hub` keyword be given parameters? (keep items/keep weapons/keep keys etc...)
- Are the Hexen sky keywords sky2, doublesky and lightning too much?
- What fog model should we use (if any)?
- The episode construct... should we add a difficulty setting too? Should the keyword be changed to `entrypoint` or something similar? (`episode` is slightly misleading)
- Should ExMy-named maps automatically be given mapnumber 10\*x+y or something like that? Maybe not.
