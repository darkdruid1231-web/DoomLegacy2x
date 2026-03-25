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
//
// \file
/// \brief Internal helper functions for menu rendering.
///
/// These are used by ProductionMenuBackend to implement slider, textbox, and
/// thermo drawing without duplicating the material/position logic.

#ifndef menu_helpers_h
#define menu_helpers_h 1

struct consvar_t;

/// Draw a slider bar at (x, y) with given range [0..100].
/// Uses M_SLIDEL, M_SLIDEM, M_SLIDER, M_SLIDEC materials and sets colormap.
void M_DrawSlider(int x, int y, int range);

/// Draw a text-box border+fill at (x, y) with given pixel dimensions.
/// Uses window_border[] patches and window_background.
void M_DrawTextBox(int x, int y, int width, int height);

/// Draw a "thermo" display for a cvar at (x, y).
/// Uses M_SLDLT, M_SLDRT, M_SLDMD1, M_SLDMD2 (raven) or M_THERML, etc.
void M_DrawThermo(int x, int y, consvar_t *cv);

#endif // menu_helpers_h
