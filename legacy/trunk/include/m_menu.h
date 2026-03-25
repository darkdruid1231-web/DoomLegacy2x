// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: m_menu.h 449 2007-05-05 20:45:25Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2007 by DooM Legacy Team.
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
/// \brief Menu system.

#ifndef m_menu_h
#define m_menu_h 1

#include "doomtype.h"

//===========================================================================
//  Menu item flags
//===========================================================================

/// flags for the menu items
enum menuflag_t
{
    // 2+2 bits, union type + action subtype
    IT_NONE = 0x0,
    IT_CONTROL,
    IT_SUBMENU = 0x4,
    IT_FUNC = 0x8,
    IT_KEYHANDLER,
    IT_FULL_KEYHANDLER,
    IT_CV = 0xC,  ///< union is a consvar
    IT_CV_SLIDER,  ///< as above, instead of string value a slider is drawn
    IT_CV_BIGSLIDER,  ///< as above, but a big slider is drawn under the item
    IT_CV_TEXTBOX,  ///< <enter> opens a textbox

    IT_UNION_MASK = 0xC,
    IT_TYPE_MASK = 0xF,

    // 4 bits: display type
    IT_SPACE = 0x00,
    IT_PATCH = 0x10,
    IT_STRING = 0x20,
    IT_CSTRING = 0x30,
    IT_DISPLAY_MASK = 0xF0,

    // misc. flags
    IT_DY = 0x0100,
    IT_TEXTBOX_IN_USE = 0x0200,
    IT_DISABLED = 0x0400,
    IT_WHITE = 0x0800,
    IT_GRAY = 0x1000,
    IT_COLORMAP_MASK = IT_WHITE | IT_GRAY,
    IT_HEADER_FLAG = 0x2000,
    IT_DROPDOWN = 0x4000,

    // shorthand
    IT_OFF_BIG = IT_NONE | IT_PATCH | IT_DISABLED | IT_GRAY,
    IT_OFF = IT_NONE | IT_CSTRING | IT_DISABLED | IT_GRAY,
    IT_CONTROLSTR = IT_CONTROL | IT_CSTRING,
    IT_LINK = IT_SUBMENU | IT_STRING | IT_WHITE,
    IT_ACT = IT_FUNC | IT_PATCH,
    IT_CALL = IT_FUNC | IT_STRING,
    IT_CVAR = IT_CV | IT_STRING,
    IT_TEXTBOX = IT_CV_TEXTBOX | IT_STRING,
    IT_CVAR_DROPDOWN = IT_CV | IT_STRING | IT_DROPDOWN,
    IT_HEADER = IT_NONE | IT_CSTRING | IT_DISABLED | IT_WHITE | IT_HEADER_FLAG,
};

typedef void (*menufunc_t)(int choice);

/// \brief Describes a single menu item
struct menuitem_t
{
    friend class TextBox;

  public:
    short flags;  ///< bit flags
    const char *pic;  ///< Texture name or NULL
    const char *text;  ///< plain text, used when we have a valid font
    byte alphaKey;  ///< hotkey in menu OR y offset of the item

  private:
    union
    {
        class Menu *submenu;   // IT_SUBMENU
        menufunc_t func;       // IT_FUNC
        struct consvar_t *cvar; // IT_CV
    };

  public:
    menuitem_t() : flags(0), pic(NULL), text(NULL), alphaKey(0), submenu(NULL) {}

    // Validating constructors (defined out-of-line in menu.cpp)
    menuitem_t(short f, const char *p, const char *t, byte a = 0);
    menuitem_t(short f, const char *p, const char *t, Menu *sm, byte a);
    menuitem_t(short f, const char *p, const char *t, menufunc_t r, byte a);
    menuitem_t(short f, const char *p, const char *t, consvar_t *cv, byte a);

    Menu *GetMenu() const
    {
        return ((flags & IT_UNION_MASK) == IT_SUBMENU) ? submenu : NULL;
    }
    menufunc_t GetFunc() const
    {
        return ((flags & IT_UNION_MASK) == IT_FUNC) ? func : NULL;
    }
    consvar_t *GetCV() const
    {
        return ((flags & IT_UNION_MASK) == IT_CV) ? cvar : NULL;
    }

    void SetFunc(menufunc_t f)
    {
        func = f;
        flags &= ~IT_TYPE_MASK;
        flags |= IT_FUNC;
    }

    void SetMenu(Menu *m)
    {
        submenu = m;
        flags &= ~IT_TYPE_MASK;
        flags |= IT_SUBMENU;
    }

    void SetCV(consvar_t *cv)
    {
        cvar = cv;
        flags &= ~IT_TYPE_MASK;
        flags |= IT_CV;
    }
};

/// \brief Game menu, may contain submenus
class Menu
{
    friend class MsgBox;
    friend class ProductionMenuBackend;

  private:
    // video and audio resources
    static class font_t *font;

    static short AnimCount;            ///< skull animation counter
    static class Material *pointer[2]; ///< the menu pointer
    static int which_pointer;
    static int SkullBaseLump; ///< menu animation base lump for Heretic and Hexen
    static tic_t NowTic;      ///< current time in tics

    static Menu *currentMenu; ///< currently active menu
    static short itemOn;      ///< currently highlighted menu item

    /// Rendering backend used by DrawMenu() ( injectable for testing ).
    static class IMenuBackend *s_menuBackend;

    typedef void (Menu::*drawfunc_t)();
    typedef bool (*quitfunc_t)();

  private:
    const char *titlepic;     ///< title Texture name
    const char *title;        ///< title as string for display with bigfont if present
    Menu *parent;             ///< previous menu
    short numitems;           ///< # of menu items
    struct menuitem_t *items; ///< array of menu items
    short lastOn;             ///< last active item
    short x, y;               ///< menu screen coords
    drawfunc_t drawroutine;   ///< draw routine
    quitfunc_t quitroutine;   ///< called before quit a menu return true if we can

  public:
    static bool active; ///< menu is currently open

    /// constructor
    Menu(const char *tpic,
         const char *t,
         Menu *up,
         int nitems,
         menuitem_t *items,
         short mx,
         short my,
         short on = 0,
         drawfunc_t df = NULL,
         quitfunc_t qf = NULL);

    /// opens the menu
    static void Open();

    /// closes the menu
    static void Close(bool callexitmenufunc);

    /// tics the menu (skull cursor) animation and video mode testing
    static void Ticker();

    /// eats events
    static bool Responder(struct event_t *ev);

    /// starts up the menu system
    static void Startup();

    /// toggles the modern sub-menu UI (main menu unchanged)
    static void SetNewUI(bool enabled);
    /// returns true if modern sub-menu UI is enabled
    static bool UsingNewUI();

    /// resets the menu system according to current game.mode
    static void Init();

    /// Sets the rendering backend used by DrawMenu() (for testing injection).
    /// Should be called before DrawMenu() in test code.
    static void SetMenuBackend(class IMenuBackend *backend);

    /// Testing seam: sets the menu font (bypasses normal Startup() init).
    /// Used by test_menu_draw to inject a null font so DrawTitle is a no-op.
    static void SetFontForTesting(class font_t *f);

    /// Testing seam: overrides the new-ui flag for testing the classic path.
    static void SetNewUIForTesting(bool enabled);

    /// Testing seam: sets AnimCount (controls cursor blink in DrawMenu).
    static void SetAnimCountForTesting(short ac);

    /// draws the menus directly into the screen buffer.
    static void Drawer();

    /// changes menu node
    static void SetupNextMenu(Menu *m);

    /// submenu event handler
    bool MenuResponder(int key);

    /// utility
    int GetNumitems() const
    {
        return numitems;
    }

    /// the actual drawing
    void DrawTitle();
    void DrawMenu();
    void DrawMenuModern();

    /// Testing seam: calls DrawMenu() with controlled state.
    /// menu_use_newui=false, Menu::font=null, AnimCount=0 are set before the call.
    void DrawMenuForTesting();

    void HereticMainMenuDrawer();
    void HexenMainMenuDrawer();
    void DrawClass();
    void DrawConnect();
    void DrawSetupPlayer();
    void DrawVideoMode();
    void DrawReadThis1();
    void DrawReadThis2();
    void DrawSave();
    void DrawLoad();
    void DrawControl();
    void DrawOpenGL();
    void OGL_DrawFog();
    void OGL_DrawColor();
};

// opens the menu and creates a message box
void M_StartMessage(const char *str);

// show or hide the setup for player 2 (called at splitscreen change)
void M_SwitchSplitscreen();

// Called by linux_x/i_video_xshm.c
void M_QuitResponse(int ch);

#endif
