# Doom Legacy 2.0 Console Reference

Make sure you read this! You will discover a lot of goodies available in Legacy! Be a Doom power user!
Skip to [section 2](#console-commands-and-variables) and browse through the list of commands if you're in a hurry!

## Contents

1. [Console basics](#console-basics)
   1. [Console usage](#console-usage)
   2. [Console input](#console-input)
   3. [Commands and variables](#commands-and-variables)
   4. [Configuration files](#configuration-files)
   5. [Autoexec.cfg](#autoexeccfg)
   6. [Script files](#script-files)
2. [Console commands and variables](#console-commands-and-variables)
   1. [Basic commands](#basic-commands)
   2. [Console setup](#console-setup)
   3. [Player setup](#player-setup)
   4. [Player controls](#player-controls)
   5. [Game options](#game-options)
   6. [Game commands](#game-commands)
   7. [Net options](#net-options)
   8. [Video options](#video-options)
   9. [Sound options](#sound-options)

---

## Console basics

### Console usage

With Doom Legacy's console, you will be able to change a lot of game parameters from within the game, and customise it to your preferences. The console is simply a command prompt, where you can enter commands and change the values of game variables. Since the first game console we know of was from id Software's game Quake, we have been inspired by it and have tried to implement the same functionality for the benefit of Quake/Doom fans.

To open the console, press the **console key**. It is usually located just below ESC. You can set the console key at the Setup Controls menu. To exit the console, either press again the console key, or press **ESC**, which will bring up the menu, and shut the console.

The console displays all messages from the game. You can go back in the messages with key **PAGE UP**, and go down with the key **PAGE DOWN**. Press the key **HOME** to jump to the oldest message in the console text buffer, and key **END** to jump to the last output message (the most recent).

The last lines of the console text buffer are displayed at the top of the screen, when the console is off. That is, you can see 5 lines of messages instead of only 1 in the original Doom. You can change the duration of display of the messages with the console variable [con_hudtime](#con_hudtime), see the commands section for more.

### Console input

While the console is on, it accepts typed-in commands. **BACKSPACE** erases the last input character, and **ENTER** executes the command. For convenience, you have a history of the last 32 commands entered. Press **UP ARROW** to go back in the commands history, **DOWN ARROW** to go forward.

Another very useful key is **TAB**: it completes a partially entered command or variable name. This is a very good way to see the existing commands: just enter any letter, then press **TAB** several times to see what commands are available.

> The variables to customize the console itself usually start with "con_", thus typing "con" and pressing the **TAB** key several times will display the following console variables: con_height, con_speed, con_hudtime, con_backpic...

You can also press **SHIFT-TAB**, which will complete the command line, but in reverse order.

### Commands and variables

Basically, a **command** does not have a value, and is not saved to the configuration file. A **variable** always has an associated value, for example the variable `name` holds your name as a player.

Entering a command without any parameters will usually tell you the command syntax.

```
> map
Usage: map <number|name> [<entrypoint>]: warp players to a map.
```

Entering a variable name alone will tell you the current value of the variable.

```
> name
"name" is "gi joe"
```

To set the value of a variable, give the new value as a parameter. When the value has blanks in it, use double quotes around it. For example, to change your name to 'dj fab', type:

```
> name "dj fab"
```

Boolean variables (true/false, yes/no) use 0 to denote false. Any nonzero value means true.

### Configuration files

Most variables are automatically saved to the default configuration file **config.cfg**. Like in the original Doom, the [-config](legacy.md#command-line-options) parameter will set explicitly the configuration file to use. Thus, you can have default settings for different persons, using different configuration files. While the configuration files support any commands, and are executed like a script, you should not edit them, because they are always overwritten when the game exits.

### Autoexec.cfg

The **autoexec.cfg** file is a text 'script' file which, if found, is automatically executed at the game startup. You can edit autoexec.cfg to set the values of those variables that are not automatically saved to config.cfg. You can also add commands that will be executed one after another, just like a script.

Comments start with `//`. Each line of the autoexec.cfg file is like a line that you would have typed at the console.

This sample 'autoexec.cfg' will warp you to map01 as soon as the game is loaded:

```
// always start with map01, without waiting at the menu
map map01
echo "welcome to map 01"   //useless comment
```

Another example, here we set the variables that are not saved to config.cfg:

```
// set my preferred weird game mode
bloodtime 5   // blood lasts 5 seconds
fastmonsters 1
teamdamage 1
echo "bigjim's weird mode active"
```

In the last example, each time you start the game, it will set the variables like [bloodtime](#bloodtime) that are not normally saved to config.cfg. Just look at config.cfg to find out which variables are saved.

### Script files

config.cfg and autoexec.cfg are script files. It means they contain commands and variables that are executed in sequence. You can create any number of script files, and execute them from the console using the [exec](#basic-commands) command.

For example, to start a script named blood.cfg just enter:

```
> exec blood.cfg
```

---

## Console commands and variables

This chapter describes all console commands and variables in Doom Legacy. Many console variables can also be modified through the menu. You will need to use the console only if you want to customise the game more to your needs.

### Basic commands

Here are the basic commands for script execution. They have the same usage and functionality as in Quake. You can use a **semicolon** to separate several commands in the same line.

| Command | Definition |
|---|---|
| `alias <aliasname> <command>` | Define an alias. When you type `<aliasname>`, it will be like if you had entered `<command>`. Example: `alias silence "soundvolume 0; musicvolume 0"` |
| `echo <text>` | Just echoes (prints) the text to console. |
| `exec <filename>` | Execute a script file. A script file is just a text file containing a sequence of commands or variables. Script files usually have a `.cfg` extension. Example: `exec autoexec.cfg` |
| `help [<variable>]` | Shows a list of all the console commands and variables. If given a variable name, shows detailed info on that variable. |
| `toggle <variable> [<value>]` | Without the `<value>` parameter this toggles a variable, i.e. changes it to the next possible value. With the `<value>` parameter it adds `<value>` to the variable. Useful with bind. Example: `bind ">" "toggle gr_fogdensity"` |
| `wait [<ticks>]` | Wait a given amount of time before the next command is executed. The unit is 1/35 seconds. Example: `wait 175; echo "ok!"` waits 5 seconds then displays "ok!". |
| `bind <keyname> <command>` | Bind commands to a key. Each time you press the key, the commands will be executed. Example: `bind s screenshot` — now each time you press s the game saves a screenshot. |
| `cls` | Clear the console text buffer. |

### Console setup

These variables control the look and feel of the console.

| Variable | Type | Definition |
|---|---|---|
| `con_backpic` | bool | 0: use a translucent LCD-like console background. 1: use an opaque [console background picture](editing.md#legacy-wad-contents), like in Quake. |
| `con_height` | int | How many percent of the total screen height the console will use. E.g. `con_height 50` (the default) will use half the screen height. |
| `con_speed` | int | The speed (in pixels/tic) at which the console comes down or moves out of the screen when you press the console toggle key. |
| `con_hudtime` <a name="con_hudtime"></a> | int | The number of seconds messages stay on the top of the screen. There are 5 message lines that scroll as soon as new messages arrive. `con_hudtime 5` (default) keeps messages for 5 seconds. Set to 0 to never see messages outside the console. |
| `showmessages <0-2>` | int | 0: never show any messages on the screen. 1: always show the messages (default). 2: show all messages except most "picked up ..." style messages. |

### Player setup

These variables control your player avatar's appearance and your game preferences. The splitscreenplayer has similar variables, but with the number 2 appended to the variable name, e.g. `color2 red` turns the splitscreenplayer's avatar red.

| Variable | Type | Definition |
|---|---|---|
| `splitscreen` | bool | Enable/disable the splitscreen. If necessary, this spawns a secondary local player, called the splitscreenplayer. |
| `name` | string | Enter your name as a player, for network games. Use `""` if the name contains spaces, e.g. `name "dj fabbe"` |
| `color <0-10>` | int | Choose your avatar's color. |
| `skin` | string | Change your skin, provided some skins are loaded. The default marine skin name is "marine". |
| `autoaim` | bool | The original Doom used autoaiming in the vertical direction. Now that you can freely aim up and down with the freelook, you can play like a pro and set autoaim off. NETWORK NOTE: the server can force autoaim off for all players with the command `allowautoaim`. |
| `originalweaponswitch` | bool | Choose whether to use the original weapon change method (1), or the new preferred weapons order (0), see `weaponpref` for more. With the original weapon switch, whenever you pick up a new weapon it becomes the active one, even if it is not as powerful as the one you are currently using. |
| `weaponpref` | string | The preferred weapons order. This works only if `originalweaponswitch` is 0. Give a priority to each weapon with a digit 0-9. You give the command a string of priority values in this order: Fist, Chainsaw, Pistol, Shotgun, Super Shotgun, Chaingun, Rocket Launcher, Plasma Rifle, BFG. Example: `weaponpref "021485763"` means super shotgun has highest priority (8). `weaponpref "333333333"` disables automatic weapon switching. |
| `crosshair <0-3>` | int | Choose whether to use a crosshair. Options: no crosshair (0), white cross (1), green angle (2), red point (3). |

### Player controls

These variables control the way the game handles the players' input. The splitscreenplayer has similar variables, but with the number 2 appended.

| Variable | Type | Definition |
|---|---|---|
| `autorun` | bool | Set to 1 to invert the run key. You will always be running, EXCEPT when you push the run key. |
| `automlook` | bool | Setting this to 1 makes the freelook permanent. When 0, you need to hold the freelook key down while moving the mouse up/down. When releasing the key, the view always re-centers. |
| `use_mouse` | bool | Setting to 0 disables the mouse. Set to 1 to search for a mouse, and activate it. |
| `invertmouse` | bool | Set to 1 to invert the mouse y (up/down) axis. For freelook only. |
| `mousemove` | bool | Enable/disable mouse y-axis for forward/backward movement. |
| `mousesensx` | int | Sets the mouse sensitivity in the x direction. |
| `mousesensy` | int | Sets the mouse sensitivity in the y direction. |
| `controlperkey <1\|2>` | int | When set to 2 (several) you can bind more than one control to a key. |
| `mouse2port <1\|2\|3\|4>` | int | Choose the port number of the secondary mouse for the splitscreenplayer between COM1 to COM4. |
| `use_joystick` | bool | Enable/disable joystick support. |
| `joystickfreelook` | bool | Enable/disable freelook with the joystick. |

The game controls can be defined using the setcontrol command.

| Command | Definition |
|---|---|
| `setcontrol <control> <key1> [<key2>]` | Binds a key/joystick button/mouse button to a game control. The second keyname is optional. If a key does not have a useful name, it uses a name of the form `KEYxxx`. The controls are: `forward, backward, strafe, straferight, strafeleft, speed, turnleft, turnright, fire, use, lookup, lookdown, centerview, mouseaiming, weapon1..weapon8, talkkey, scores, jump, console`. Example: `setcontrol forward "keypad 8" "up arrow"` |
| `setcontrol2 <control> <key1> [<key2>]` | Same as setcontrol but for splitscreenplayer. |

### Game options

Using these variables you can change many aspects of the game itself. For obvious reasons, in a netgame only the server can change them.

| Variable | Type | Definition |
|---|---|---|
| `deathmatch <0\|1\|2\|3>` | int | Set the game type: 0=coop, 1/2/3=different kinds of deathmatch. |
| `teamplay <0\|1\|2>` | int | 0: no teams, free-for-all. 1: teamplay, use color to identify teams. 2: teamskin, using skins to identify teams. |
| `teamdamage` | bool | Whether members of the same team can hurt one another. |
| `hiddenplayers` | bool | Are the other players visible in the automap? Also determines if you can switch to their POV using spy mode. Usually enabled in deathmatch. |
| `exitmode` | int | How map exits work: 0=disabled, 1=everyone exits when first activated, 2=only activating player exits, 3=nobody exits until EVERYONE has reached an exit. |
| `fraglimit` | int | When the limit is reached, the level is exited. Use 0 to disable. |
| `timelimit` | int | After the given number of minutes, the level is exited. Use 0 to disable. |
| `gravity` | float | Sets the acceleration of gravity in the game. Default is 1. |
| `nomonsters` | bool | Set to 1 if you want no monsters to spoil your gaming experience. Often used in deathmatch. |
| `fastmonsters` | bool | Choose if monsters are "fast" like with the original -fastparm. |
| `solidcorpse` | bool | Setting to 1 makes dead corpses solid. They stack and you can climb on them to reach ledges! |
| `allowjump` | bool | Choose whether jumping is allowed. NOTE: Jumping is considered cheating in maps not designed with it in mind. |
| `allowrocketjump` | bool | Enable/disable emulation of the Quake1 "Rocket Jump" bug. |
| `allowautoaim` | bool | Enable/disable autoaiming for all players in the game. |
| `allowfreelook` | bool | Enable/disable looking up and down for all players in the game. |
| `respawnitem` | bool | Choose whether items respawn or not. `deathmatch 2` automatically sets it to 1. |
| `respawnitemtime` | int | Set the respawn item time in seconds. Default is 30 seconds. |
| `respawnmonsters` | bool | Choose if monsters can respawn. Like the original -respawn command-line option. |
| `respawnmonsterstime` | int | Choose the time of respawn for monsters, in seconds. Default is 12. |

### Game commands

Here we describe the commands that can be used during the game. They are also the primary way a dedicated server interacts with the game.

| Command | Definition |
|---|---|
| `save <slot_number> <description>` | Save the game in the given slot, with chosen description. |
| `load <slot_number>` | Loads a previously saved game. |
| `exitlevel` | Exits current level and goes to the intermission screen. |
| `restartlevel` | Restart the current level. |
| `pause` | Pauses/unpauses the game (or requests a pause from the server). |
| `quit` | Quits the game without further confirmation. |
| `version` | Shows the game version and build date. |
| `saveconfig <filename>` | Save the current config in the specified file. This is always done when you exit Legacy. |
| `loadconfig <filename>` | Load a config file like at the beginning of Legacy. |
| `changeconfig <filename>` | Do a savegame of current config, then load the specified filename. |
| `screenshot` | Take a screenshot. |
| `meminfo` | Show the amount of heap, virtual and physical memory available. |
| `gameinfo` | Prints information about the current game. |
| `mapinfo` | Prints information about the current map. |
| `players` | Prints information about the players. |
| `frags` | Shows the frags table. |
| `say <message>` | Sends a message to all players. |
| `sayto <playername\|playernum> <message>` | Sends a message to a specified player. |
| `sayteam <message>` | Sends a message to your own team. |
| `chatmacro <0-9> <message>` | Change the chat messages. Press the chat key, then ALT+number (0-9) to send one of the chat messages. |
| `connect <server_ip \| server_address> \| any` | Connects to a server to play a netgame. "connect any" searches for servers and connects to the first one found. |
| `disconnect` | Disconnects from the server. |
| `kick <playername \| playernum>` | Kick somebody out of your game! |
| `kill me \| <playername \| playernum> \| monsters` | Kills just about anything. |
| `map <number \| lumpname> [<ep>]` | Warps all the players to a map. ep is the requested entrypoint within the map. |
| `addfile playdemo demospeed stopdemo recorddemo restartgame resetgame runacs t_dumpscript t_runscript t_running addbot` | Various commands. |
| `noclip` | Enables/disables clipping for the console player. Like the cheat code **idclip**. |
| `fly` | Enables/disables fly mode for the console player. In fly mode the player can fly using the **jump** control. |
| `god` | God mode. Just like the cheat **iddqd**. |
| `gimme health \| ammo \| weapons \| armor \| keys \| map \| almostanything...` | The cheat à la carte. |

### Net options

| Variable | Type | Definition |
|---|---|---|
| `servername` | string | This is the name of your server. It is displayed on clients' screens when they are searching for servers. |
| `masterserver` | string | The masterserver address used during server search and when registering your server. |
| `publicserver` | bool | If true, your server will announce itself to the master server so that it will be visible also outside the local network. |
| `allownewplayers` | bool | Allows or disallows the joining of new players to the game. |
| `maxplayers` | int | Maximum allowed number of players for the game. Default is 32. |
| `netstat` | bool | |

### Video options

These variables control many visual aspects of the game.

| Variable | Type | Definition |
|---|---|---|
| `fullscreen` | bool | Choose between fullscreen and windowed mode. |
| `scr_depth <8\|16\|24\|32>` | int | Screen color depth in bits. |
| `scr_width` | int | Screen width in pixels. |
| `scr_height` | int | Screen height in pixels. |
| `usegamma <0-4>` | int | Choose the gamma level (brightness). |
| `viewsize <3-11>` | int | Choose the size of the game viewport. 3=smallest view window. 10=full viewsize with status bar. 11=full viewsize without status bar. |
| `viewheight <16-56>` | int | Set the height of the viewpoint (the height of the eyes above the floor). Normal value for the Doom marine is 41. |
| `chasecam` | bool | Enable/disable the chasecam. |
| `cam_height` | float | Sets the chasecam height. |
| `cam_dist` | float | Sets the distance between the chasecam and the player. |
| `cam_speed` | float | Sets the speed of the chasecam. |
| `bloodtime` <a name="bloodtime"></a> | int | Choose how many seconds blood will stay on the floor. In the original Doom the blood stayed no more than 1 second. Setting to 60 makes it stay 1 minute — useful for tracking down your prey :) |
| `translucency` | bool | Enable/disable translucent sprites. |
| `screenlink <0\|1\|2>` | int | Change the screen link between 0=none, 1=normal melting, 2=color melt. |
| `playersprites` | bool | Choose whether to draw the player weapon sprites. |
| `splats` | bool | Enable/disable bloodsplats and bullet holes on walls. |
| `scalestatusbar` | bool | Should the status bar graphics be scaled? |
| `hud_overlay` | string | Choose what information to show in the HUD when in full screen mode. Each letter represents a piece of information: `f`=frags (deathmatch only), `h`=health, `m`=armor, `a`=current weapon ammo, `k`=keys, `e`=monsters killed, `s`=secrets found. |
| `framerate` | bool | When true, shows the current framerate (frames/s) in the game window. |
| `fuzzymode` | bool | |

The following options only apply in OpenGL mode.

| Variable | Type | Definition |
|---|---|---|
| `gr_coronas` | bool | Enable/disable coronas around light sources. |
| `gr_coronasize` | float | Sets the corona size. You can adjust the size to get a more realistic effect. |
| `gr_dynamiclighting` | bool | Dynamic lighting renders on walls and planes a diffuse reflection of nearby light sources, such as missiles or BFG balls. Can be slow on low end PCs. See also `gr_mblighting`. |
| `gr_mblighting` | bool | Same as dynamic lighting, except it shows the light emitted by missiles thrown by monsters. |
| `gr_fog` | bool | Enable/disable the fog, default is on. |
| `gr_fogcolor` | string | Change the RGB (in hexadecimal format, "rrggbb") color of the fog. Default is `000000` (black). For a grey fog, set to `707070`. |
| `gr_fogdensity` | float | Change the fog density. Default value is 500. |
| `gr_fov` | float | Field of view, in degrees. If you set a value above 90 and `gr_mlook` is "Off", then Legacy will automatically set `gr_mlook` to "On". Useful for zooming. |
| `gr_gammared \| gr_gammagreen \| gr_gammablue` | int | Set the intensities of red, green and blue on the scale 0-255. |
| `gr_mlook` | int | Fixes the "mlook" bug when looking up and down. 0=Off (only if you don't use full mlook), 1=On (fast, recommended for low-end PCs), 2=Full (same as On but fixes all bugs, slightly slower for angles > 45°). |
| `gr_staticlighting` | bool | Experimental static lighting variable. Not yet optimized. |

### Sound options

Variables controlling sound and music.

| Variable | Type | Definition |
|---|---|---|
| `soundvolume <0-31>` | int | Sound effects volume. |
| `musicvolume <0-31>` | int | Music volume. |
| `cd_volume <0-31>` | int | CD music volume. |
| `snd_channels` | int | Choose how many sound channels will be used. Usually a value from 8 to 16 is enough. |
| `stereoreverse` | bool | Reverse the roles of the right and left speakers? |
| `surround` | bool | Use a software "surround" effect? |
| `precachesound` | bool | Should all sounds be loaded at the map startup, or only when they're needed? |
