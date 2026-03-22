# Doom Legacy Map Author's Reference

**Version 2.0**

## Contents

1. [Editing Basics](#editing-basics)
   - [Legacy data files](#legacy-data-files)
2. [Maps](#maps)
   - [The MAPINFO lump](#the-mapinfo-lump)
   - [LevelInfo lumps](#levelinfo-lumps)
   - [Map formats](#map-formats)
   - [New THING types](#new-thing-types)
   - [New LINEDEF types](#new-linedef-types)
   - [3D floors and water](#3d-floors-and-water)
3. [Media](#media)
   - [Textures](#textures)
   - [Sprites](#sprites)
   - [3D models](#3d-models)
   - [Colormaps et al.](#colormaps-et-al)
   - [Sound](#sound)
   - [Music](#music)
4. [Advanced topics](#advanced-topics)
   - [Scripting](#scripting)
   - [DECORATE](#decorate)
   - [DeHackEd and BEX](#dehacked-and-bex)
5. [Legacy.wad contents](#legacywad-contents)

---

# Editing Basics

This section explains the basic things you need to know if you want to edit or author Doom Legacy maps.

## Legacy data files

The original Doom stores all game data (maps, graphics, sounds) in data archives called WAD files. The WAD format is somewhat restricting, mainly due to the 8 character lumpnames and the lack of internal directory structure or compression. More recent games have introduced other data archive formats which remedy some of these shortcomings. In addition to WADs, Legacy understands the WAD2/3, PAK and ZIP/PK3 file formats. Data files are included in the game using the commandline switch **-file**:

```
-file <filename1> [filename2...]
```

You can also use normal directories to emulate data files by entering the name of a directory after the **-file** switch. The name must end with a slash (/). In this case, each file within the directory emulates a lump. This can be very useful while testing new textures, sounds etc. Since there is no universally defined ordering among the files in a directory, you cannot load maps (which consist of several lumps in a very specific order) from them.

Finally, the **-file** switch also accepts files whose name ends with ".lmp". They will be treated as single lumps whose name is given by the filename (minus the ".lmp").

### Which data file format to use?

The WAD2/3 and PAK formats are only meant to be used if you want to use an existing data archive and don't want to repackage it. When creating something new from scratch, the question boils down to choosing between WAD and ZIP. ZIP archives have three main advantages over WADs:

1. DEFLATE compression. ZIPs are generally smaller in size than corresponding WADs.
2. Almost arbitrarily long lumpnames and directory-like structures. However, because most resource names in the Doom/Hexen map format are limited to 8 characters, this feature is of limited use.
3. One does not need WAD editing tools to make ZIP archives, any ZIP program will do.

They also have a disadvantage:

1. Slight performance penalty due to having to decompress the lumps when they are used.

NOTE: Modern data formats such as PNG, JPEG, MP3 or Ogg Vorbis already are compressed and you will not get any further benefit by recompressing them. Thus, if you only want to make a texture package with PNG and JPEG textures, a WAD file is generally a better choice than a ZIP file. However, if you can force your zipping program just to "store" already compressed files in the ZIP file without compressing ("deflating") them again, the ZIP format will be as efficient as a WAD. With the UNIX commandline tool **zip** you do this using the `-n` switch.

---

# Maps

This section explains how Legacy maps can differ from Vanilla Doom ones.

## Map formats

Legacy understands both Doom and Hexen map formats. If a map has a BEHAVIOR lump (even an empty one), it is assumed to be in Hexen format. You can freely mix the formats in a map WAD, i.e. have some of the maps in Doom format and others in Hexen format.

The only difference between Doom and Hexen map formats is in the THINGS and LINEDEFS lumps. Legacy includes a utility console command for converting Doom-format maps to Hexen format:

```
convertmap <maplumpname>
```

The converted lumps are written on disk with the names `<maplumpname>_THINGS.lmp` and `<maplumpname>_LINEDEFS.lmp`. You can then replace the original THINGS and LINEDEFS lumps in the WAD with the converted versions using almost any WAD editor.

Boom and Heretic maps follow the Doom map format, only some THING and LINEDEF numbers have new meanings. Legacy's default interpretation of the THING and LINEDEF numbers depends on the IWAD used.

## The MAPINFO lump

MAPINFO is a special plaintext lump, introduced in Hexen, extended by ZDoom and later the JDS group, that fixes many shortcomings of the map format and can be used *inter alia* to create custom map orderings and hubs within the WAD. Legacy follows the [JDS MAPINFO standard](JDS_MAPINFO.md).

There is some overlap in the functionality of the MAPINFO lump and the [LevelInfo](#levelinfo-lumps) lumps. We suggest that you use MAPINFO whenever you can, and LevelInfo when you must.

Usually there should be only one MAPINFO lump per wad, containing entries for each map and cluster in the game. For examples of MAPINFO lumps, see the **MI_*** lumps in legacy.wad. For instance, [this lump](../resources/MAPINFO_Doom.txt) recreates the original Doom episodes.

## LevelInfo lumps

The LevelInfo lumps are plain text files, stored in the **map separator lump**, which has the same name as the map (e.g. E1M4, MAP21) and is normally empty. Each map can have its own LevelInfo. The lump consists of one or more **blocks**. Each block begins with:

```
[this is a blockname]
```

and ends when the next block begins, or when the lump ends. The recognized blocks are **[level info]**, **[scripts]** and **[intertext]**.

### [level info]

This block holds information concerning the entire map. It consists of commands, one per line.

| [level info] command | Effect |
|---|---|
| `consolecmd <expr>` | **Deprecated**. Execute a [console expression](console.md) after loading the map. |
| **Deprecated commands** (use [MAPINFO](#the-mapinfo-lump) instead) | |
| `name <nicename>` | Sets the full name of the map (e.g. "Mission Control"). |
| `version <string>` | Map version. |
| `author <name>` | Name of the map author. |
| `description <string>` | An additional string, which can be a map description, hint, recommendation etc. |
| `partime <n>` | Map partime (in seconds). |
| `music <lumpname>` | Sets the music played in the map. |
| `gravity <float>` | Sets the acceleration of gravity in the map. Default is 1.0. |

### [scripts]

This block contains the [FraggleScript](fsbasic.md) scripts of the map.

### [intertext]

Contains the text to be shown after the map is exited. `\n` is the newline character.

## New THING types

The Doom (and Hexen) map format refers to different types of THINGs (a.k.a. mapthings, map objects, mobjs) by unsigned 16-bit codes, known as DoomEd numbers. Doom, Heretic and Hexen all use different numberings, with the number ranges overlapping in some places. Legacy tries to honor the most common "new" thing types, especially those introduced by Boom and SMMU.

| THING type | Description | Origin |
|---|---|---|
| 4001-4028 | Playerstarts for players 5-32. | Boom |
| 5001, 5002 | Point push and pull effects | Boom |
| 5003 | Camera location | SMMU |
| 5004 | Path node | |
| 500x | TODO Teamstartsec | |
| 8090 | Skybox Viewpoint | ZDoom |
| 8091 | Skybox Picker | ZDoom |
| 9300 | Polyobj anchor | ZDoom |
| 9301 | Polyobj spawn spot | ZDoom |
| 9302 | Crushing polyobj spawn spot | ZDoom |

## New LINEDEF types

Legacy supports all Boom linedef types. For the details, see the [Boom reference](boomref.html). Additionally, we have introduced a number of our own additions to the Doom linedef system. These additions are also available in Hexen-format maps through the Hexen linedef type **150**, see below.

In Doom-format maps, we have introduced a new linedef flag, `Alltrigger [0x400] (1024)`. This flag, when set, will allow all things to activate a W* line, with the exception of flying blood. This includes all generalized types that are W*.

### FraggleScript triggers

| LINEDEF type | Effect |
|---|---|
| 272 | Start FS script (tagnumber), WR |
| 273 | Start FS script (tagnumber), WR, 1-sided |
| 274 | Start FS script (tagnumber), W1 |
| 275 | Start FS script (tagnumber), W1, 1-sided |
| 276 | Start FS script (tagnumber), SR |
| 277 | Start FS script (tagnumber), S1 |
| 278 | Start FS script (tagnumber), GR |
| 279 | Start FS script (tagnumber), G1 |

### Visual effects

| LINEDEF type | Effect |
|---|---|
| 280 | **Swimmable Boom water:** Works like Boom's 242 type but with major differences. 280's water is swimmable (using the Legacy swimming code). Water is only shown if it is above the target sector's floor. The lightlevel of the water is controlled by the target sector's lightlevel. 280 also puts colormaps in the targeted sectors, based on the upper/lower/middle textures. NOTE: 280 will also create a fake ceiling if the control ceiling is lower than the target ceiling (this is usually not desired). |
| 282 | **Easy colormap/fog effect:** Sets the colormaps of all tagged sectors. The colormaps are generated by Legacy at runtime. Upper texture: `#rrggbba` (6 hex chars for RGB color, 1 alphanumeric for amount). Middle texture: `#abbcc` (a=fog boolean, bb=fade start 0-32, cc=destination 1-33). Lower texture: `#rrggbb` (fade destination color). |
| 283 | **Fog sheet:** The middle texture will be rendered as if fully transparent, only applying the relevant colormap to anything seen through it. Only affects sides with a middle texture. |
| 284 | Software translucency — TRANSMED: Brightens everything behind the line (kinda green). |
| 285 | Software translucency — TRANSMOR: Less brightness with less green. |
| 286 | Software translucency — TRANSHI: Darkens a little with no green tint. |
| 287 | Software translucency — TRANSFIR: Brightens but with no green tint. |
| 288 | Software translucency — TRANSFX1: Selective translucency. Only certain colors are translucent. |

### Fake floors, aka 3D floors

These are one of Doom Legacy's most interesting features. A fake floor is defined by its bottom and top heights, the horizontal dimensions of the sector it is in, and its type. The volume inside the fake floor can be filled with solid, swimmable water, fog etc.

A fake floor is created using a **control sector** — usually a simple small triangular sector not connected to the rest of the map. One of the linedefs surrounding the control sector is tagged and given a specific type from the list below. This causes all sectors with the same tag to contain a fake floor.

The bottom and top heights of the fake floor are determined by the floor and ceiling heights of the control sector. The bottom and top textures of the fake floor are the floor and ceiling textures of the control sector. The middle texture of the linedef creating the fake floor is used for the texture along the sides. The lightlevel and colormap *under* the fake floor correspond to the control sector's lightlevel and colormap.

| LINEDEF type | Effect |
|---|---|
| 281 | Opaque solid 3D floor. |
| 289 | Opaque solid 3D floor which does not change the lighting conditions below it. |
| 300 | Translucent solid 3D floor. Upper texture field holds the alpha value: [#000..#255] (#000=transparent, #255=opaque, default #127). |
| 306 | Invisible solid 3D floor. |
| 301 | Translucent swimmable water. Upper texture field holds the alpha value. |
| 304 | Opaque swimmable water. |
| 302 | Volumetric fog, uses the control sector's colormap. |
| 303 | 3D light at control sector's ceiling height. |
| 305 | 3D light at control sector's ceiling and floor height. |

### Hexen-format maps

All the new linedef types can also be used in Hexen-format maps. We have extended the Hexen linedef system by one new type, 150. Under this type, the arg1 and arg2 fields determine the subtype of the effect. Also, you can use full 16-bit tags with linedeftype 150. arg4 holds the low byte and arg5 the high byte of the tag.

| LINEDEF type | arg1 | arg2 | Effect |
|---|---|---|---|
| 150 | 4: Exotic use of texture names | 1 | Boom 242 |
| 150 | 4: Exotic use of texture names | 2 | Boom 260 |
| 150 | 4: Exotic use of texture names | 3 | Legacy 280 |
| 150 | 4: Exotic use of texture names | 4 | Legacy 282 |
| 150 | 10: Fake floors | type | Legacy (type + 281) |
| 150 | 11: Renderer effects | 0 | Fog sheet |
| 150 | 11: Renderer effects | 100+n | Software translucency: transmap n |
| 150 | 128: FraggleScript trigger | onesided | If onesided is 1, line can only be activated from the first side. |

## 3D floors and water

*The concept of true 3D in Doom Legacy. By Steven McGranahan.*

### Chapter 1: The Beginning

Close to a year ago, the DosDoom team shocked the Doom community with an animated gif of a doom level with a real, true 3D bridge that the player could go under and over. Now, DosDoom has become EDGE and true 3D has become one of its main features.

About 8 months ago, Steven McGranahan began a series of experiments designed to simulate true 3D in Doom Legacy, which eventually led to invisible solid regions that objects could stand on top of and below. When the EDGE team released their code, the rendering code was successfully ported to Legacy, and then significantly extended to support multiple 3D floors per sector, shadow casting, and better sprite sorting. About 80% of the 3D floor code is now original Legacy code.

### Chapter 2: The lowdown

3D floors in Legacy are not perfect. They are plagued by bugs in the original Doom rendering system, and many visual bugs are not fixable without overhauling the rendering system. These bugs, however, are now very few and far between.

There are known speed issues. A 3D floor will slowdown Legacy about as much as adding 3 sectors. Slowdowns are caused mainly by overdraw — extra scene components that Doom has to draw (3D sides and extra floor/ceiling planes). Slowdown is generally related to two factors: number of stacked 3D floors in a sector, and the number of subsectors in a sector.

### Chapter 3: Your first level with 3D floors

To create a 3D bridge:

1. Build a small 5-sector level: Sectors A and B (floor=0, ceiling=128, light=255) with Sectors C, D, E (ceiling=128, floor=-64, light=255, lava/nukage floor) in between.
2. Create a control sector F (small triangle, off-map). Set floor height=-8, ceiling height=0, both textures to FLOOR4_8, light=176.
3. Select one of sector F's linedefs, set the line special to 281 (solid, opaque, 3D floor) and tag it to sector C.

Now sector F will be a "model" for a 3D floor in sector C. Sector F's floor becomes the bottom of the 3D floor, its ceiling becomes the top, the middle texture of the linedef 281 becomes the sides, and sector F's lightlevel becomes the shadow intensity.

### Chapter 4: What causes slowdowns

Slowdowns are caused mainly by overdraw. Sorting takes all the sprites and 3D floors and arranges them so they will draw in the correct order. Most of the time this works flawlessly.

Controlling the number of subsectors a sector has is pretty much impossible, because different node builders will choose different partition lines. However, you can control the number of linedefs in a 3D floor. Keeping 3D objects simple will help reduce rendering times.

---

# Media

This section describes all the non-map data used by Legacy: textures, sounds, music, 3D models, sprites, colormaps etc.

## Textures

Any and all of the sprites, patches, textures and flats in the IWAD can be replaced using PWADs. In the following, we use the word "texture" for all graphics, not just the ones defined in the TEXTUREx lumps.

The Legacy texture system is somewhat involved, caused by two main reasons: The original Doom graphics formats have no [magic numbers](http://en.wikipedia.org/wiki/File_format#Magic_number) by which they could be reliably identified, and there are certain nasty texture namespace overlaps in the original IWADs. Legacy follows the [JDS texture standard](http://glbsp.sourceforge.net/JointTextureSpec.txt).

### Graphics formats

Doom natively supports two graphics formats: patches and flats. Doom Legacy also supports **PNG** and **JPEG** image formats.

**Flats** are raw paletted images stored in a single lump. The lump only contains a `width*height`-sized array of one-byte palette indices. Flats implicitly use the PLAYPAL palette. Flats contain no magic numbers and hence have to reside between the **F_START** and **F_END** (or FF_START and FF_END) markers. The supported flat sizes are 64×64, 128×64 and 128×128 texels. Flats cannot be used as parts of a sprite.

**Patches** are a more advanced graphics format that the original Doom used for building wall textures, sprites and menu images. A patch consists of vertical runs of pixels possibly separated by holes. Patches implicitly use the PLAYPAL palette. Each patch is stored in its own lump. They have no magic numbers.

**PNG** and **JPEG** (more accurately JPEG/JFIF) are modern graphics formats that have magic numbers. They require no external resources like a palette. They both support high color images and are compressed. Moreover, PNG images can have an alpha channel which enables variable transparency. In Doom Legacy, each PNG or JPEG image should be stored in its own lump.

A good rule of the thumb: if you've drawn the texture by hand, or it has transparency effects, save it as PNG. If it's based on a scanned image or a digital photo, it may work better as JPEG.

### Texture definitions

Before a graphic can be used in a game, the game engine has to convert it into a texture. One important attribute of a texture is its **scaling**, i.e. how many texels are rendered over unit map/screen length. By default the x and y scales are 1.

**Doom Textures** are the textures defined in the TEXTURE* and PNAMES lumps. They are composed of one or more patches. There are some unused zero fields in the texture definition structs. One of these has been adopted to set the scaling of the texture.

The `maptexture_t` struct:

| Field | Description |
|---|---|
| char[8] | texture name |
| Uint16 | bit flags (currently unused, zero) |
| 2 × Uint8 | x scale, y scale in 5.3 fixed point format. If zero, scale is 1. |
| 2 × Uint16 | texture width, height in pixels |
| Uint32 | unused, zero |
| Uint16 | patchcount |

**F_START textures** are textures consisting of a single flat. Each lump between **F_START** and **F_END** markers is converted into a F_START texture with the same name as the lump.

**S_START textures** are textures consisting of a single patch or an image with magic numbers. Each lump between **S_START** and **S_END** markers is converted into a S_START texture with the same name as the lump. Used exclusively for sprite frames.

**TX_START textures** are built out of lumps residing between **TX_START** and **TX_END** markers. Each of these lumps must contain a single graphic in a known image format that has magic numbers. TX_START is the quickest and easiest way to add high-color textures to your WAD.

**H_START textures** are single-lump textures which must have magic numbers and reside between **H_START** and **H_END** markers. They work just like TX_START textures, except the texture is scaled so that its worldsize matches a Doom Texture or F_START texture with the same name. H_START textures are meant for hi-res texture packs.

**NTEXTURE textures** are the most advanced and versatile type of textures. They are defined in a plaintext lump named NTEXTURE. It contains any number of texture definitions like this:

```
// this is a comment
texture MYNAME1 // the texture name
{
    data LUMPNAME; // name of the lump containing the image
    worldsize X Y; // the world size into which the texture is scaled
    offset DX DY;  // optional internal offsets for the texture
}
```

**NSPRITES textures** are just like NTEXTURE textures, except that the definition lump is called NSPRITES. Used exclusively for sprite frames.

### Animated textures

#### ANIMDEFS lump

TODO explain

#### ANIMATED lump

TODO

## Sprites

Sprites are the objects used to represent mapthings in Doom. Each sprite consists of several frames, each of which is a texture.

All the sprite definition lumps should be between the **S_START** and **S_END** (or SS_START and SS_END) markers. The markers prevent nasty errors stemming from the fact that sprite lumps have a special naming convention in which the first four characters define the sprite.

When the sprite definition lumps are loaded, Legacy first looks for an NSPRITES texture by that name, and then a S_START texture. You can leave the spriteframe lumps empty if you supply the corresponding textures somewhere else — the lumps are only needed to declare the existence of the particular frames.

## 3D models

3D models are what modern FPS games use to represent non-fixed map objects. Legacy knows how to use MD3 models — the ones from Quake III Arena. The MD3s are assigned to different mapthings using [DECORATE](#decorate) lumps. MD3 skins are technically textures.

## Colormaps et al.

The Doom software renderer uses many different kinds of color mapping tables to simulate lighting and color blending effects. The tables consist of indices referring to palette colors.

A **colormap** is a 256-byte array used to remap the colors inside a palette: `newcolor = colormap[oldcolor]`.

The Doom software renderer uses a set of 34 different colormaps, called a **fadetable** or a lighttable, to simulate different visibility levels. The default fadetable is stored in the confusingly named COLORMAP lump. One can also simulate colored lighting and fog using fadetables.

A **translation table** is a special kind of colormap which usually only remaps a narrow range of colors inside a palette. Translation tables are used to generate differently colored versions of the sprites (in the case of the Doom marine, this happens by remapping the green color range in the palette to something else).

A **translucency map** is a 256×256 byte array used for selective software translucency, where a background color is seen through a translucent foreground: `newcolor = transmap[fg_color][bg_color]`. You can visualize it as a set of 256 colormaps, one for each foreground color. Boom (and hence Legacy) uses the lump TRANSMAP as the default translucency map.

TODO: C_START, C_END

## Sound

### Audio formats

Legacy supports two sound formats: the native Doom one and **WAV/RIFF**.

### SNDINFO lump

SNDINFO is a plaintext lump which defines the sound effects in the game. Unlike normal WAD resources, SNDINFO lumps are handled in a **cumulative**, not overriding, fashion. The [SNDINFO lump](../resources/SNDINFO.txt) in **legacy.wad** defines the basic sound effect naming in Legacy. You can freely define new sound effects or change the existing ones by using your own SNDINFO lumps.

Comments begin with a semicolon and end at the next newline.

| SNDINFO command | Effect |
|---|---|
| `<sndname> <lumpname>` `[m<n>] [r<n>] [p<n>]` | Assigns the sound in the lump `<lumpname>` to the logical sound name `<sndname>`. Optionally, you can also set the **m**ultiplicity, p**r**iority or the **p**itch of the effect. |
| `<sndname> = <alias>` | Defines an alias for a logical sound name. Mainly for Hexen compatibility. |
| `$multiplicity <0-255>` | Set the default multiplicity of the consequent sound effects. Zero means not limited. |
| `$priority <0-255>` | Set the default relative importance of the sound effects. |
| `$pitch <-128 - +127>` | Set the default pitch change for the sounds. 64 pitch units equal an octave. |
| `$ifdoom` | Starts a conditional block that is only executed when in Doom mode. |
| `$ifheretic` | Conditional block executed only in Heretic mode. |
| `$ifhexen` | Conditional block executed only in Hexen mode. |
| `$endif` | Ends a conditional block. |

### SNDSEQ lump

The SNDSEQ lump is used to define sequences of sound effects for doors, moving platforms, ambiance etc. You can think of them as very simple scripts. All the SNDSEQ commands refer to logical sound names, as defined in SNDINFO. Like SNDINFO, SNDSEQ lumps are read in a cumulative fashion. See the [SNDSEQ lump](../resources/SNDSEQ.txt) in legacy.wad for an example.

Semicolons start comments, which end at the next newline.

| SNDSEQ command | Effect |
|---|---|
| `:<seqname> <number>` | Starts a sound sequence definition. The beginning colon is essential. Each sequence must have a unique (positive) number. |
| `play <sndname>` | Starts playing the sound effect, moves immediately to the next command. |
| `delay <ticks>` | Waits a number of ticks before continuing the script. |
| `delayrand <min> <max>` | Waits a random number of ticks before continuing the script. |
| `playtime <sndname> <ticks>` | Like a play command followed by a delay command. |
| `playuntildone <sndname>` | Starts playing the sound, moves forward when it has finished. |
| `playrepeat <sndname>` | Starts playing the sound repeatedly. This effectively halts the script. |
| `volume <0-100>` | Sets the sound volume. |
| `volumerand <min> <max>` | Sets the sound volume randomly. |
| `chvol <n>` | Changes the sound volume by the relative amount n. |
| `stopsound <sndname>` | Defines the sound to be played when the sequence is stopped (e.g. the sound of a door slamming shut). |
| `end` | Ends the sound sequence definition. |

## Music

Legacy supports the MUS, MIDI, MP3 and Ogg Vorbis music formats, as well as several module formats, using the SDL_mixer library. The music lump format is detected automagically. The names of the music lumps can be **chosen freely** — they do not have to start with "D_".

---

# Advanced topics

This section handles some of the more advanced editing topics, such as FS and ACS scripting and the DECORATE and DeHackEd data definition languages.

## Scripting

Legacy supports two scripting languages, [FraggleScript](fsbasic.md) and ACS. Both languages can be used in all types of maps, even simultaneously if needed. FraggleScript is stored in the **[scripts]** blocks of the [LevelInfo](#levelinfo-lumps) lumps, whereas ACS goes to the **BEHAVIOR** lumps. FS is an interpreted language and hence can be inserted into the wad as plaintext, whereas ACS must first be compiled into bytecode using **ACC**, the ACS compiler.

## DECORATE

DECORATE is a language for defining mapthing properties, introduced by ZDoom. TODO

## DeHackEd and BEX

Legacy supports the most important DeHackEd and BEX (Boom EXtensions to DeHackEd) features. Some of the features are **deprecated**, which means that you should not use them since there is a better way to achieve the same effect.

| Feature | Support status | Notes |
|---|---|---|
| Thing | full | BEX flag mnemonics are supported, with extensions. |
| Frame | full | Extension: Codepointer can be set using `codep <codepointer_mnemonic>`. |
| Weapon | full | |
| Ammo | full | |
| Sprite | full | |
| Misc | partial | |
| Cheat | partial | |
| Pointer | deprecated | Use [CODEPTR] or the Frame extension instead. |
| Sound | deprecated | Use [SNDINFO](#sndinfo-lump) instead. |
| Text | deprecated | Use [STRINGS] or [MAPINFO](#the-mapinfo-lump) instead. |
| [CODEPTR] | full | Extended with a few Legacy-specific codepointers. |
| [STRINGS] | partial | Use [MAPINFO](#the-mapinfo-lump) for intermission texts and nice mapnames. |
| [PARS] | deprecated | Use [MAPINFO](#the-mapinfo-lump) instead. |

### Frames and codepointers

In Doom, the mapthing AI is basically a finite state machine. The behavior and animation of the things is controlled by a **state table** (frame table), which is an array of **states** (frames). Each state consists of: **sprite** number, **frame** number (sprite subnumber), duration in **tics**, **nextstate** number (next frame) and **action function** pointer (codepointer).

The sprite and frame numbers determine the graphical presentation. Duration is self-explanatory. Nextstate is the number of the state the thing will move into next. The action function pointer points to an action function that will be called on the thing each time it enters the state. The pointer can also be NULL, in which case no function is called.

The preferred way to change the codepointers is to use the BEX mnemonics. The complete list of mnemonics for the Doom, Heretic and Hexen action functions available can be found in the source file [a_functions.h](../include/a_functions.h). Remember that you can use the mnemonic **NULL** to clear a codepointer.

Legacy also has introduced a few new action functions, which can only be accessed using their mnemonics:

| BEX mnemonic | Effect |
|---|---|
| `StartFS` | Starts an FS script, using **tics** for the script number. The actual duration of the state is always zero. |
| `StartWeaponFS` | Same thing, but for weapon frames. |
| `StartACS` | Starts an ACS script, using **tics** for the script number, **frame** for the args??? TODO |
| `StartWeaponACS` | Same as above, but for weapon frames. |

### Item pickups

If a mapthing has the flag **SPECIAL**, the player may pick it up. Unlike the original Doom, Legacy does not determine the type of a mapthing by its sprite, but rather by its mobjtype number. Where applicable, the **health** field of an item mapthing determines the pickup's "size" (health, armor points or ammo).

You can also create completely new types of pickups by combining DeHackEd with scripting. If a mapthing has both the SPECIAL flag and a nonzero **missile damage**, it becomes a scripted pickup. When the player touches it, the game runs the FS script whose number corresponds to the missile damage of the mapthing. Original hardcoded behavior, if any, is completely bypassed.

TODO: more howtos: creating a new weapon with scripting, new monsters...

### DeHackEd with Heretic and Hexen frames

The Heretic and Hexen frame tables are also included in Legacy. They are always available, even in pure Doom games. One can refer to the Heretic frames using the letter **H** as a prefix to the frame number. Similarly, the Hexen states are referred to using the letter **X** as a prefix. This allows you to modify Heretic and Hexen games using DeHackEd, but you can also simply use the extra states for creating more complex mapthing behavior.

### Caveat

DeHackEd, even when improved with BEX, is somewhat clumsy, convoluted and unappealing to novice map authors. Since most "oldskool" map authors like it and plenty of WADs with DeHackEd content are available, we'll try to support it in future releases as well. However, eventually we intend to add more [DECORATE](#decorate) support, which will hopefully obsolete DeHackEd altogether.

Here is an [example DEH file](example.deh) which illustrates the new features.

---

# Legacy.wad contents

Naturally, you can also replace these resources using PWADs.

| Lump | Contents |
|---|---|
| **Plaintext lumps** | |
| [VERSION](../resources/VERSION.txt) | Legacy.wad version string |
| [MI_DOOM1](../resources/MAPINFO_Doom.txt) | MAPINFO lump for Doom1 |
| [MI_DOOM2](../resources/MAPINFO_Doom2.txt) | MAPINFO lump for Doom2 |
| [MI_TNT](../resources/MAPINFO_Evilution.txt) | MAPINFO lump for the Doom2 mission pack "TNT Evilution" |
| [MI_PLUT](../resources/MAPINFO_Plutonia.txt) | MAPINFO lump for the Doom2 mission pack "Plutonia Experiment" |
| [MI_HTIC](../resources/MAPINFO_Heretic.txt) | MAPINFO lump for Heretic |
| [SNDINFO](../resources/SNDINFO.txt) | Basic sound effect definitions |
| [SNDSEQ](../resources/SNDSEQ.txt) | Basic sound sequence definitions |
| [THINGS.H](../resources/FS_things.h) | FraggleScript header file containing THING names and flags |
| ENDOOM | End text |
| **Binary lumps** | |
| [XDOOM](../resources/doom2hexen.txt) | Doom-to-Hexen linedeftype conversion table. |
| [XHERETIC](../resources/heretic2hexen.txt) | Heretic-to-Hexen linedeftype conversion table |
| **Default data items** | |
| DEF_SND | Default sound effect |
| [DEF_TEX](../resources/DEF_texture.png) | Default texture |
| **Fadetables and translucency maps** | |
| WATERMAP | Boom underwater lightmap/fadetable. |
| TRANSMED, TRANSMOR, TRANSHI, TRANSFIR, TRANSFX1 | Maps for default software translucency. |
| **Sounds** | |
| DSOUCH, DSJUMP, DSFLOUSH, DSGLOOP, DSSPLASH | New player and environment sounds |
| **Sprites** | |
| BLUD[ab]0 | Blood |
| SMOK[a-e]0 | Smoke |
| SPLA[a-c]0 | Water splash? |
| TNT1A0 | ? |
| **Other graphics** | |
| CREDIT | Credits page |
| CORONA | Corona luminosity map |
| CROSHAI[1-3] | HUD crosshairs |
| BRDR_MM, BRDR_MT | Window border patches? |
| A_DMG[1-3] | Wall splat patches |
| WATER[0-7] | "old water" flats |
| M_CONTROL, M_CDVOL, M_VIDEO, M_SINGLE, M_MULTI, M_SETUPM, M_SETUPA, M_SETUPB, M_2PLAYR, M_STSERV, M_CONNEC, M_SLIDEL, M_SLIDER, M_SLIDEC | Menu patches |
| RANKINGS | The graphic displayed at the top of the deathmatch rankings (while in a net-game) |
| RSKY[1-3] | The sky graphics are now 256×240, not 256×128. That's because of the freelook: you are able to look higher, thus seeing a part of the sky you couldn't see in the original Doom. The bottom 40 lines are under the horizon and not normally seen. The lines 100-199 are the ones you see while looking straight ahead. The top lines 0-99 are the lines of sky you see when looking up. Legacy scales up the SKY textures if it finds that they are the old small size 256×128. |
| CONSBACK | Console background picture, 320×200, when 'con_backpic' is true. Also used as the loading picture. MUST BE 200 lines high, can be any width (default is 320). |
| CBLEFT | Console left border, used with translucent console background. MUST BE 200 lines high, any width (default is 8). |
| CBRIGHT | Console right border, used with translucent console background. MUST BE 200 lines high, any width (default is 8). |
| **Status bar overlay icons** | |
| SBOHEALT | Health (default shows a red cross) |
| SBOFRAGS | Frags (default shows a skull) |
| SBOARMOR | Armor (default shows a green armor) |
| SBOAMMO1 | Bullets |
| SBOAMMO2 | Shotgun shells |
| SBOAMMO3 | Power cells |
| SBOAMMO4 | Rockets |
