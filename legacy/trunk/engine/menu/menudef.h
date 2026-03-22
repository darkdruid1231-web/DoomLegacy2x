// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2026 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief MENUDEF lump parser — Phase 3 of the widget-based menu system.
///
/// Mods can include a MENUDEF lump (plain text) in a WAD or PK3 to declare
/// menus without recompiling engine code.  The parser produces WidgetPanel
/// objects registered by name; DrawMenuModern() checks the registry by menu
/// title before falling back to the C++ MenuToWidgetPanel() adapter.
///
/// Supported MENUDEF syntax:
///
///   OptionMenu "MenuTitle"
///   {
///       Header  "Section label"
///       Separator
///       Separator "Optional label"
///       Option  "Label", "cvar_name"
///       Slider  "Label", "cvar_name", minval, maxval, step
///       Input   "Label", "cvar_name"
///       Button  "Label", "submenu_or_func"
///       Space
///       Image   "texname"
///       Image   "texname", width, height
///   }
///
/// Multiple OptionMenu blocks may appear in one lump (and across multiple lumps
/// — all MENUDEF lumps in the loaded WAD stack are parsed).
///
/// Cvar names must match a registered consvar_t (via consvar_t::FindVar()).
/// Unknown cvars produce a warning and are skipped.

#ifndef menu_menudef_h
#define menu_menudef_h 1

class WidgetPanel;

/// Parse all MENUDEF lumps found in the WAD file cache.
/// Call this after MAPINFO loading (and after all WADs are loaded).
void MenuDef_LoadAll();

/// Look up a previously parsed panel by menu title.
/// Returns NULL if no MENUDEF panel was registered under that name.
WidgetPanel *MenuDef_FindPanel(const char *title);

/// Free all parsed panels (called on shutdown / WAD reload).
void MenuDef_Clear();

#endif // menu_menudef_h
