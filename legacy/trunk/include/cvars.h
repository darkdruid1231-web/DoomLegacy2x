// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: cvars.h 570 2009-11-27 23:57:47Z smite-meister $
//
// Copyright (C) 2004-2006 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Console variable declarations.

#ifndef cvars_h
#define cvars_h 1

struct consvar_t;

// misc
extern consvar_t cv_masterserver;
extern consvar_t cv_netstat;

// server info
extern consvar_t cv_servername;
extern consvar_t cv_publicserver;
extern consvar_t cv_allownewplayers;
extern consvar_t cv_maxplayers;
// extern consvar_t cv_maxteams;

// server demo
extern consvar_t cv_playdemospeed;

// server rules
extern consvar_t cv_deathmatch;
extern consvar_t cv_teamplay;
extern consvar_t cv_teamdamage;
extern consvar_t cv_hiddenplayers;

extern consvar_t cv_exitmode;
extern consvar_t cv_intermission;
extern consvar_t cv_fraglimit;
extern consvar_t cv_timelimit;

extern consvar_t cv_jumpspeed;
extern consvar_t cv_fallingdamage;
extern consvar_t cv_allowrocketjump;
extern consvar_t cv_allowautoaim;
extern consvar_t cv_allowmlook;
extern consvar_t cv_allowpause;

extern consvar_t cv_itemrespawn;
extern consvar_t cv_itemrespawntime;
extern consvar_t cv_respawnmonsters;
extern consvar_t cv_respawnmonsterstime;
extern consvar_t cv_fragsweaponfalling;
extern consvar_t cv_bodyqueue_size;
extern consvar_t cv_bodyqueue_monsters;

extern consvar_t cv_gravity;
extern consvar_t cv_nomonsters;
extern consvar_t cv_fastmonsters;
extern consvar_t cv_solidcorpse;
extern consvar_t cv_voodoodolls;
extern consvar_t cv_infighting;

// client info (server needs to know)
extern consvar_t cv_splitscreen;

// client info shared by players
extern consvar_t cv_cam_dist;
extern consvar_t cv_cam_height;
extern consvar_t cv_cam_speed;

// client input (g_input.cpp)
extern consvar_t cv_grabinput;
extern consvar_t cv_controlperkey;
extern consvar_t cv_usemouse[2];
extern consvar_t cv_mousesensx[2];
extern consvar_t cv_mousesensy[2];
extern consvar_t cv_automlook[2];
extern consvar_t cv_mousemove[2];
extern consvar_t cv_invertmouse[2];
extern consvar_t cv_mouse2port;
#ifdef LMOUSE2
extern consvar_t cv_mouse2opt;
#endif

// client console (console.cpp)
extern consvar_t cons_msgtimeout;
extern consvar_t cons_speed;
extern consvar_t cons_height;
extern consvar_t cons_backpic;
extern consvar_t cons_conwidth;
extern consvar_t cons_alpha;
extern consvar_t cons_logfile;

// client hud (hu_stuff.cpp)
extern consvar_t cv_stbaroverlay;

// client chat (hu_stuff.cpp)
extern consvar_t cv_chatmacro1;
extern consvar_t cv_chatmacro2;
extern consvar_t cv_chatmacro3;
extern consvar_t cv_chatmacro4;
extern consvar_t cv_chatmacro5;
extern consvar_t cv_chatmacro6;
extern consvar_t cv_chatmacro7;
extern consvar_t cv_chatmacro8;
extern consvar_t cv_chatmacro9;
extern consvar_t cv_chatmacro0;

// client audio (s_sound.cpp)
extern consvar_t cv_soundvolume;
extern consvar_t cv_musicvolume;
extern consvar_t cd_volume;
extern consvar_t cv_numChannels;
extern consvar_t cv_stereoreverse;
extern consvar_t cv_surround;
extern consvar_t cv_precachesound;

// client renderer
extern consvar_t cv_viewheight;

extern consvar_t cv_scr_width;
extern consvar_t cv_scr_height;
extern consvar_t cv_scr_depth;
extern consvar_t cv_fullscreen;
extern consvar_t cv_aspectratio;
extern consvar_t cv_video_gamma;

extern consvar_t cv_viewsize;
extern consvar_t cv_detaillevel;
extern consvar_t cv_scalestatusbar;
extern consvar_t cv_fov;

extern consvar_t cv_screenslink;
extern consvar_t cv_translucency;
extern consvar_t cv_fuzzymode;
extern consvar_t cv_splats;
extern consvar_t cv_bloodtime;
extern consvar_t cv_psprites;

// client opengl renderer
extern consvar_t cv_grsolvetjoin;
extern consvar_t cv_grrounddown;
extern consvar_t cv_grcrappymlook;
extern consvar_t cv_grdynamiclighting;
extern consvar_t cv_grstaticlighting;
extern consvar_t cv_grshadows;
extern consvar_t cv_grmblighting;
extern consvar_t cv_grcoronas;
extern consvar_t cv_grcoronasize;
extern consvar_t cv_monball_light;
extern consvar_t cv_granisotropy;
extern consvar_t cv_grpolygonsmooth;
extern consvar_t cv_grmd2;
extern consvar_t cv_grtranswall;
extern consvar_t cv_grfog;
extern consvar_t cv_grfogcolor;
extern consvar_t cv_grfogdensity;
extern consvar_t cv_grnormalmapstrength;
extern consvar_t cv_grcontrast;
extern consvar_t cv_grgammared;
extern consvar_t cv_grgammagreen;
extern consvar_t cv_grgammablue;
extern consvar_t cv_grfiltermode;
extern consvar_t cv_grcorrecttricks;
extern consvar_t cv_grnearclippingplane;
extern consvar_t cv_grfarclippingplane;
extern consvar_t cv_grfiltermode;

// modern graphics quality controls
extern consvar_t cv_vsync;
extern consvar_t cv_fpslimit;
extern consvar_t cv_glquality;
extern consvar_t cv_msaa;

// post-processing effects
extern consvar_t cv_grbloom;
extern consvar_t cv_grbloomthreshold;
extern consvar_t cv_grbloomstrength;
extern consvar_t cv_grssao;
extern consvar_t cv_grssaostrength;
extern consvar_t cv_grdeferred; ///< Enable G-buffer MRT + deferred dynamic light accumulation (Phase 3.1).
extern consvar_t cv_grcubeshadows; ///< Number of deferred lights that receive omnidirectional cube shadow maps (Phase 3.2). 0 = off, 1-4.
extern consvar_t cv_grssr;         ///< Enable screen-space reflections (Phase 4.1). Requires deferred mode.
extern consvar_t cv_grssrstrength; ///< SSR blend strength [0,1] (Phase 4.1).
extern consvar_t cv_grvolfog;      ///< Enable PostFX exponential volumetric fog (Phase 4.2).
extern consvar_t cv_grfxaa;        ///< FXAA anti-aliasing (Phase 5.2).
extern consvar_t cv_grgodrays;         ///< Volumetric light shafts (Phase 5.3).
extern consvar_t cv_grgodraysstrength; ///< God rays blend strength [0,1] (Phase 5.3).
extern consvar_t cv_grsunazimuth;      ///< Sun compass direction in degrees 0–359 (Phase 5.3).
extern consvar_t cv_grsunaltitude;     ///< Sun elevation in degrees 0–89 (Phase 5.3).
extern consvar_t cv_grspecular;    ///< Blinn-Phong specular strength [0,1] (Phase 5.1).
extern consvar_t cv_grspecularexp; ///< Blinn-Phong specular exponent [4,128] (Phase 5.1).
#endif
