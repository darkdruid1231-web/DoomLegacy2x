// Single-player stubs implementation
// Provides stub implementations for networking functions

#ifdef SINGLEPLAYER

#include "am_map.h"
#include "command.h"
#include "console.h"
#include "cvars.h"
#include "doomdef.h"
#include "doomtype.h"
#include "g_game.h"
#include "g_player.h"
#include "hud.h"
#include "m_menu.h"
#include "r_main.h"
#include "s_sound.h"
#include "screen.h"
#include "sounds.h"
#include "v_video.h"
#include "w_wad.h"

// CVAR stubs - provide definitions
consvar_t cv_fragsweaponfalling = {"cv_fragsweaponfalling", "", 0};
consvar_t cv_allowpause = {"cv_allowpause", "", 1};
consvar_t cv_hiddenplayers = {"cv_hiddenplayers", "", 0};
consvar_t cv_maxplayers = {"cv_maxplayers", "", 8};
consvar_t cv_allownewplayers = {"cv_allownewplayers", "", 1};
consvar_t cv_masterserver = {"cv_masterserver", "", 0};
consvar_t cv_servername = {"cv_servername", "My Doom Server", 0};
consvar_t cv_publicserver = {"cv_publicserver", "", 0};
// Note: cv_menu_serversearch is defined in menu.cpp

// Additional CVARs that might be needed
consvar_t cv_allowrocketjump = {"cv_allowrocketjump", "", 1};
consvar_t cv_itemrespawn = {"cv_itemrespawn", "", 0};

// Game mode CVARs (from net/sv_main.cpp and others)
consvar_t cv_deathmatch = {"deathmatch", "0", 0};
consvar_t cv_fraglimit = {"fraglimit", "0", 0};
consvar_t cv_teamplay = {"teamplay", "0", 0};
consvar_t cv_timelimit = {"timelimit", "0", 0};
consvar_t cv_nomonsters = {"nomonsters", "0", 0};
consvar_t cv_fastmonsters = {"fastmonsters", "0", 0};
consvar_t cv_respawnmonsters = {"respawnmonsters", "0", 0};
consvar_t cv_allowmlook = {"allowfreelook", "1", 0};
consvar_t cv_exitmode = {"exitmode", "0", 0};
consvar_t cv_itemrespawntime = {"itemrespawntime", "0", 0};
consvar_t cv_bodyqueue_size = {"bodyqueue_size", "8", 0};
consvar_t cv_intermission = {"intermission", "0", 0};
consvar_t cv_voodoodolls = {"voodoodolls", "1", 0};
consvar_t cv_respawnmonsterstime = {"respawnmonsterstime", "0", 0};
consvar_t cv_gravity = {"gravity", "1.0", 0};
consvar_t cv_fallingdamage = {"fallingdamage", "1", 0};
consvar_t cv_allowautoaim = {"allowautoaim", "1", 0};
consvar_t cv_jumpspeed = {"jumpspeed", "0", 0};
consvar_t cv_bodyqueue_monsters = {"bodyqueue_monsters", "3", 0};
consvar_t cv_solidcorpse = {"solidcorpse", "1", 0};
consvar_t cv_teamdamage = {"teamdamage", "0", 0};
consvar_t cv_infighting = {"infighting", "1", 0};
consvar_t cv_splitscreen = {"splitscreen", "0", 0};

// Stub implementations for networking functions
void SV_Init()
{
    // Single-player - no server needed
}

bool GameInfo::Playing()
{
    return state == GS_LEVEL;
}

void GameInfo::SV_Reset(bool clear_mapinfo)
{
    (void)clear_mapinfo;
    // Reset game state for single-player
}

bool GameInfo::SV_SpawnServer(bool force_mapinfo)
{
    (void)force_mapinfo;
    // Single-player always has a "server"
    return true;
}

void GameInfo::SV_SetServerState(bool open)
{
    (void)open;
    // Single-player doesn't have server states
}

bool GameInfo::SV_StartGame(skill_t skill, int mapnumber, int ep)
{
    (void)skill;
    (void)mapnumber;
    (void)ep;
    // In single-player, just start the game
    return true;
}

void GameInfo::CL_Reset()
{
    // Reset client state - not needed in single-player
}

void GameInfo::TryRunTics(unsigned int tics)
{
    (void)tics;
    // Single-player tic handling
}

void GameInfo::ReadResourceLumps()
{
    // Single-player - load resources normally
}

// LocalPlayerInfo stub
void LocalPlayerInfo::UpdatePreferences()
{
    // Single-player - nothing to update
}

// PlayerInfo stub
void PlayerInfo::Ticker()
{
    // Single-player - update player state
}

// CL_Init stub - needed by d_main.cpp
void CL_Init()
{
    CONS_Printf("\n============ CL_Init (single-player) ============\n");

    // set the video mode, graphics scaling properties, load palette
    vid.Startup();

    // init renderer
    CONS_Printf("CL_Init: calling R_Init\n");
    R_Init();
    CONS_Printf("CL_Init: R_Init done\n");

    CONS_Printf("CL_Init: calling font_t::Init\n");
    font_t::Init();
    CONS_Printf("CL_Init: font_t::Init done\n");

    // we need the HUD font for the console
    // HUD font, crosshairs, say commands
    CONS_Printf("CL_Init: calling hud.Startup\n");
    hud.Startup();
    CONS_Printf("CL_Init: hud.Startup done\n");

    // startup console
    CONS_Printf("CL_Init: calling con.Init\n");
    con.Init(); //-------------------------------------- CONSOLE is on
    CONS_Printf("CL_Init: con.Init done\n");

    // setup menu
    CONS_Printf("CL_Init: calling Menu::Startup\n");
    Menu::Startup();
    CONS_Printf("CL_Init: Menu::Startup done\n");

    CONS_Printf("CL_Init: calling automap.Startup\n");
    automap.Startup();
    CONS_Printf("CL_Init: automap.Startup done\n");

    // set up sound and music
    CONS_Printf("CL_Init: calling S.Startup\n");
    S.Startup();
    CONS_Printf("CL_Init: S.Startup done\n");

    // read the basic legacy.wad sound script lumps
    S_Read_SNDINFO(fc.FindNumForNameFile("SNDINFO", 0));
    S_Read_SNDSEQ(fc.FindNumForNameFile("SNDSEQ", 0));

    // Register client cvars that would normally be registered in CL_Init
    cv_viewsize.Reg();
    cv_scalestatusbar.Reg();
    cv_fov.Reg();
    cv_gr_fov.Reg();
    cv_splitscreen.Reg();
}

// OpenGL extension stubs
// These functions are from OpenGL extensions (GL_ARB_multitexture, GL_ARB_point_parameters)

#ifdef _WIN32
#include <GL/gl.h>
#include <windows.h>

// Define these as C functions that do nothing
extern "C"
{

    // GL_ARB_multitexture
    void WINAPI glActiveTexture(GLenum texture)
    {
        (void)texture;
        // Stub - does nothing
    }

    // GL_ARB_point_parameters
    void WINAPI glPointParameterf(GLenum pname, GLfloat param)
    {
        (void)pname;
        (void)param;
        // Stub - does nothing
    }

    void WINAPI glPointParameterfv(GLenum pname, const GLfloat *params)
    {
        (void)pname;
        (void)params;
        // Stub - does nothing
    }
}

#endif // _WIN32

#endif // SINGLEPLAYER
