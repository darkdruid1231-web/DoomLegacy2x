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
/// \brief Widget implementations for the modern menu renderer (Phase 1).
///
/// This file has no dependency on menuitem_t — the adapter function
/// MenuToWidgetPanel() lives in menu.cpp where that struct is fully defined.

#include "widget.h"

#include "command.h"    // consvar_t, CV_FLOAT
#include "console.h"    // whitemap, graymap
#include "doomdef.h"    // BASEVIDWIDTH / BASEVIDHEIGHT
#include "g_game.h"     // game, gm_heretic
#include "keys.h"       // KEY_UPARROW / KEY_DOWNARROW
#include "r_data.h"     // materials
#include "r_draw.h"     // window_background, window_border, BRDR_*
#include "v_video.h"    // font_t, V_SCALE, V_MAP, V_WHITEMAP, current_colormap

// IT_TEXTBOX_IN_USE bit — must match the value in menu.cpp's menuflag_t.
static const int W_IT_TEXTBOX_IN_USE = 0x0200;

bool Widget::IsConditionallyDisabled() const
{
    return enabled_if_cvar && !enabled_if_cvar->value;
}

// Slider geometry — must match SLIDER_RANGE / SLIDER_WIDTH in menu.cpp.
static const int W_SLIDER_RANGE = 10;
static const int W_MAXSTRINGLENGTH = 32;

//===========================================================================
//  Internal drawing helpers (equivalents of M_DrawSlider / M_DrawTextBox)
//===========================================================================

static void Widget_DrawSlider(int x, int y, int range)
{
    if (range < 0)   range = 0;
    if (range > 100) range = 100;

    materials.Get("M_SLIDEL")->Draw(x - 8, y, 0 | V_SCALE);
    for (int i = 0; i < W_SLIDER_RANGE; i++)
        materials.Get("M_SLIDEM")->Draw(x + i * 8, y, 0 | V_SCALE);
    materials.Get("M_SLIDER")->Draw(x + W_SLIDER_RANGE * 8, y, 0 | V_SCALE);

    current_colormap = whitemap;
    materials.Get("M_SLIDEC")->Draw(
        x + ((W_SLIDER_RANGE - 1) * 8 * range) / 100, y, V_SCALE | V_MAP);
}

// Exact replica of M_DrawTextBox from menu.cpp.
// width = number of character cells, height = number of text rows.
static void Widget_DrawTextBox(int x, int y, int width, int height)
{
    int step, boff;
    if (game.mode >= gm_heretic)
    {
        step = 16;
        boff = 4;
    }
    else
    {
        step = 8;
        boff = 8;
    }

    int cx = x - boff;
    int cy = y - boff;
    window_border[BRDR_TL]->Draw(cx, cy, V_SCALE);
    for (cy = y; cy < y + height; cy += step)
        window_border[BRDR_L]->Draw(cx, cy, V_SCALE);
    window_border[BRDR_BL]->Draw(cx, cy, V_SCALE);

    window_background->DrawFill(x, y, width, height);
    for (cx = x; cx < x + width; cx += step)
    {
        window_border[BRDR_T]->Draw(cx, y - boff, V_SCALE);
        window_border[BRDR_B]->Draw(cx, cy, V_SCALE);
    }

    window_border[BRDR_TR]->Draw(cx, y - boff, V_SCALE);
    for (cy = y; cy < y + height; cy += step)
        window_border[BRDR_R]->Draw(cx, cy, V_SCALE);
    window_border[BRDR_BR]->Draw(cx, cy, V_SCALE);
}

//===========================================================================
//  LabelWidget
//===========================================================================

void LabelWidget::Draw(const MenuDrawCtx &ctx, int dy, bool /*focused*/)
{
    if (!text)
        return;

    if (is_header)
    {
        // Header background chrome is drawn by WidgetPanel; just draw text.
        ctx.font->DrawString(ctx.panel_x + ctx.panel_padding,
                             dy + 2,
                             text,
                             V_WHITEMAP | V_SCALE);
    }
    else if (is_white)
    {
        current_colormap = whitemap;
        ctx.font->DrawString(ctx.panel_x + ctx.panel_padding, dy, text,
                             V_MAP | V_SCALE);
    }
    else
    {
        ctx.font->DrawString(ctx.panel_x + ctx.panel_padding, dy, text, V_SCALE);
    }
}

//===========================================================================
//  ButtonWidget
//===========================================================================

void ButtonWidget::Draw(const MenuDrawCtx &ctx, int dy, bool /*focused*/)
{
    if (!text)
        return;

    bool eff_disabled = disabled || IsConditionallyDisabled();
    int flags = V_SCALE;
    if (eff_disabled)
    {
        flags |= V_MAP;
        current_colormap = graymap;
    }
    ctx.font->DrawString(ctx.panel_x + ctx.panel_padding, dy, text, flags);
}

//===========================================================================
//  OptionWidget
//===========================================================================

void OptionWidget::Draw(const MenuDrawCtx &ctx, int dy, bool /*focused*/)
{
    if (!cv)
        return;

    bool eff_disabled = disabled || IsConditionallyDisabled();

    // Label
    if (text)
    {
        int flags = V_SCALE;
        if (eff_disabled) { flags |= V_MAP; current_colormap = graymap; }
        ctx.font->DrawString(ctx.panel_x + ctx.panel_padding, dy, text, flags);
    }

    int value_x = ctx.panel_x + ctx.panel_w - ctx.panel_padding - ctx.value_box_w;

    if (item_flags & 0x4000 /*IT_DROPDOWN*/)
    {
        ctx.mat_header->DrawFill(value_x, dy - 2, ctx.value_box_w, ctx.row_height - 2);
        ctx.font->DrawString(value_x + 6, dy, cv->str, V_WHITEMAP | V_SCALE);
        ctx.font->DrawString(value_x + ctx.value_box_w - 10, dy, ">",
                             V_WHITEMAP | V_SCALE);
    }
    else
    {
        int val_flags = V_WHITEMAP | V_SCALE;
        if (eff_disabled) { val_flags = V_MAP | V_SCALE; current_colormap = graymap; }
        ctx.font->DrawString(value_x, dy, cv->str, val_flags);
    }
}

//===========================================================================
//  SliderWidget
//===========================================================================

void SliderWidget::Draw(const MenuDrawCtx &ctx, int dy, bool /*focused*/)
{
    if (!cv)
        return;

    bool eff_disabled = disabled || IsConditionallyDisabled();

    if (text)
    {
        int flags = V_SCALE;
        if (eff_disabled) { flags |= V_MAP; current_colormap = graymap; }
        ctx.font->DrawString(ctx.panel_x + ctx.panel_padding, dy, text, flags);
    }

    if (!eff_disabled && cv->PossibleValue &&
        cv->PossibleValue[1].value != cv->PossibleValue[0].value)
    {
        int slider_x = ctx.panel_x + ctx.panel_w - ctx.panel_padding - ctx.slider_width;
        int range = ((cv->value - cv->PossibleValue[0].value) * 100 /
                     (cv->PossibleValue[1].value - cv->PossibleValue[0].value));
        Widget_DrawSlider(slider_x, dy, range);
    }
}

//===========================================================================
//  TextInputWidget
//===========================================================================

void TextInputWidget::Draw(const MenuDrawCtx &ctx, int dy, bool /*focused*/)
{
    if (!cv)
        return;

    int x0 = ctx.panel_x + ctx.panel_padding;

    if (text)
        ctx.font->DrawString(x0, dy, text, V_SCALE);

    Widget_DrawTextBox(x0, dy + 4, W_MAXSTRINGLENGTH, 1);

    bool in_use = item_flags_ptr && (*item_flags_ptr & W_IT_TEXTBOX_IN_USE);

    if (in_use && ctx.textbox_active && ctx.textbox_text)
    {
        ctx.font->DrawString(x0 + 8, dy + 12, ctx.textbox_text, V_SCALE);
        if (ctx.AnimCount < 4)
        {
            ctx.font->DrawCharacter(
                x0 + 8 + ctx.font->StringWidth(ctx.textbox_text),
                dy + 12, '_', V_WHITEMAP | V_SCALE);
        }
    }
    else
    {
        ctx.font->DrawString(x0 + 8, dy + 12, cv->str, V_SCALE);
    }
}

//===========================================================================
//  WidgetPanel
//===========================================================================

WidgetPanel::~WidgetPanel()
{
    for (Widget *w : widgets)
        delete w;
    widgets.clear();
}

int WidgetPanel::ItemTopY(int idx, int row_height, int header_height) const
{
    int y = 0;
    for (int i = 0; i < idx && i < (int)widgets.size(); i++)
    {
        Widget *w = widgets[i];
        if (!w) { y += row_height; continue; }
        y += w->extra_top_margin + w->Height(row_height, header_height);
    }
    return y;
}

void WidgetPanel::EnsureVisible(int idx, int row_height, int header_height, int visible_h)
{
    if (idx < 0 || idx >= Count() || visible_h <= 0)
        return;

    int top = ItemTopY(idx, row_height, header_height);
    Widget *w = At(idx);
    int h = w ? w->Height(row_height, header_height) : row_height;

    if (top < scroll_offset)
        scroll_offset = top;
    else if (top + h > scroll_offset + visible_h)
        scroll_offset = top + h - visible_h;

    if (scroll_offset < 0)
        scroll_offset = 0;
}

void WidgetPanel::Draw(const MenuDrawCtx &ctx)
{
    if (!ctx.mat_panel || !ctx.mat_header || !ctx.mat_focus || !ctx.font)
        return;

    ctx.mat_panel->DrawFill(ctx.panel_x, ctx.panel_y, ctx.panel_w, ctx.panel_h);

    // Visible content rect (inside padding).
    int clip_top    = ctx.panel_y + ctx.panel_padding;
    int clip_bottom = ctx.panel_y + ctx.panel_h - ctx.panel_padding;
    int visible_h   = clip_bottom - clip_top;

    // Auto-scroll so the focused item is always on-screen.
    EnsureVisible(ctx.itemOn, ctx.row_height, ctx.header_height, visible_h);

    // Content starts above clip_top when scrolled.
    int dy = clip_top - scroll_offset;

    // Positions of the open dropdown anchor (filled during the first pass).
    int dropdown_y = -1;
    int dropdown_x = 0;
    int dropdown_w = 0;
    consvar_t *dropdown_cv = nullptr;

    int n = Count();
    for (int i = 0; i < n; i++)
    {
        Widget *w = widgets[i];
        if (!w)
        {
            dy += ctx.row_height;
            if (dy >= clip_bottom) break;
            continue;
        }

        // IT_DY extra top offset (stored on the widget by MenuToWidgetPanel).
        dy += w->extra_top_margin;

        LabelWidget *lw = dynamic_cast<LabelWidget *>(w);
        bool is_hdr = lw && lw->is_header;
        // Use the widget's self-reported height (ListWidget overrides this).
        int  row_h  = w->Height(ctx.row_height, ctx.header_height);
        bool focused = (i == ctx.itemOn);

        // Skip rows entirely above the visible area.
        if (dy + row_h <= clip_top)
        {
            dy += row_h;
            continue;
        }
        // Stop once we're past the visible area.
        if (dy >= clip_bottom)
            break;

        // Per-row chrome (background / highlight).
        if (is_hdr)
        {
            ctx.mat_header->DrawFill(ctx.panel_x + 4,
                                     dy - 1,
                                     ctx.panel_w - 8,
                                     row_h - 2);
        }
        else if (focused && w->CanFocus())
        {
            ctx.mat_focus->DrawFill(ctx.panel_x + 4,
                                    dy - 2,
                                    ctx.panel_w - 8,
                                    row_h);
        }

        w->Draw(ctx, dy, focused);

        // Capture dropdown anchor for the overlay pass below.
        if (focused && ctx.dropdown_open && ctx.dropdown_item == i)
        {
            OptionWidget *ow = dynamic_cast<OptionWidget *>(w);
            if (ow && ow->cv)
            {
                dropdown_cv = ow->cv;
                dropdown_y  = dy;
                dropdown_x  = ctx.panel_x + ctx.panel_w - ctx.panel_padding - ctx.value_box_w;
                dropdown_w  = ctx.value_box_w;
            }
        }

        dy += row_h;
    }

    // Dropdown overlay — drawn on top of all rows.
    if (ctx.dropdown_open && dropdown_y >= 0 && dropdown_cv &&
        dropdown_cv->PossibleValue)
    {
        int count = 0;
        while (dropdown_cv->PossibleValue[count].strvalue)
            count++;

        if (count > 0)
        {
            int visible = count < 8 ? count : 8;
            int start   = ctx.dropdown_index - visible / 2;
            if (start < 0)               start = 0;
            if (start + visible > count) start = count - visible;

            int list_h = visible * ctx.row_height;
            int list_y = dropdown_y + ctx.row_height;
            if (list_y + list_h > ctx.panel_y + ctx.panel_h)
                list_y = dropdown_y - list_h;
            if (list_y < ctx.panel_y + ctx.panel_padding)
                list_y = ctx.panel_y + ctx.panel_padding;

            ctx.mat_panel->DrawFill(dropdown_x, list_y, dropdown_w, list_h);
            for (int i = 0; i < visible; i++)
            {
                int idx   = start + i;
                int row_y = list_y + i * ctx.row_height;
                if (idx == ctx.dropdown_index)
                    ctx.mat_focus->DrawFill(dropdown_x + 2,
                                            row_y - 1,
                                            dropdown_w - 4,
                                            ctx.row_height - 2);
                if (dropdown_cv->PossibleValue[idx].strvalue)
                    ctx.font->DrawString(dropdown_x + 6,
                                        row_y + 1,
                                        dropdown_cv->PossibleValue[idx].strvalue,
                                        V_WHITEMAP | V_SCALE);
            }
        }
    }

    // Footer: hint for why the focused item is disabled.
    if (ctx.disabled_reason)
    {
        int msg_y = ctx.panel_y + ctx.panel_h - 10; // footer_gap
        ctx.font->DrawString(ctx.panel_x + ctx.panel_padding,
                             msg_y,
                             ctx.disabled_reason,
                             V_WHITEMAP | V_SCALE);
    }
}

bool WidgetPanel::HandleNavKey(int key, short &itemOn, int numitems)
{
    if (key != KEY_DOWNARROW && key != KEY_UPARROW)
        return false;

    int n = Count();
    if (n == 0 || numitems == 0)
        return false;

    if (key == KEY_DOWNARROW)
    {
        int start = itemOn;
        do
        {
            itemOn = (itemOn + 1 >= numitems) ? 0 : static_cast<short>(itemOn + 1);
        } while (itemOn != start &&
                 (itemOn >= n || !widgets[itemOn] ||
                  !widgets[itemOn]->CanFocus()));
    }
    else // KEY_UPARROW
    {
        int start = itemOn;
        do
        {
            itemOn = (itemOn == 0) ? static_cast<short>(numitems - 1)
                                   : static_cast<short>(itemOn - 1);
        } while (itemOn != start &&
                 (itemOn >= n || !widgets[itemOn] ||
                  !widgets[itemOn]->CanFocus()));
    }

    return true;
}

int WidgetPanel::ContentHeight(int row_height, int header_height) const
{
    int h = 0;
    for (const Widget *w : widgets)
    {
        if (!w) { h += row_height; continue; }
        h += w->extra_top_margin + w->Height(row_height, header_height);
    }
    return h;
}

//===========================================================================
//  SeparatorWidget
//===========================================================================

void SeparatorWidget::Draw(const MenuDrawCtx &ctx, int dy, bool /*focused*/)
{
    int cx = ctx.panel_x + ctx.panel_padding;
    int cw = ctx.panel_w - ctx.panel_padding * 2;
    int cy = dy + ctx.row_height / 2 - 1;

    // Draw a simple two-pixel horizontal rule using the header material.
    ctx.mat_header->DrawFill(cx, cy, cw, 2);

    if (label)
    {
        float lw = ctx.font->StringWidth(label);
        float lx = ctx.panel_x + (ctx.panel_w - lw) / 2.0f;
        // Overdraw the centre of the rule with panel background, then text.
        ctx.mat_panel->DrawFill(static_cast<int>(lx) - 4, cy, static_cast<int>(lw) + 8, 2);
        ctx.font->DrawString(lx, dy + ctx.row_height / 2 - ctx.row_height / 4,
                             label, V_WHITEMAP | V_SCALE);
    }
}

//===========================================================================
//  ImageWidget
//===========================================================================

void ImageWidget::Draw(const MenuDrawCtx &ctx, int dy, bool /*focused*/)
{
    if (!tex_name)
        return;

    Material *mat = materials.Get(tex_name);
    if (!mat)
        return;

    int x0 = ctx.panel_x + ctx.panel_padding;
    if (img_w > 0 && img_h > 0)
        mat->DrawFill(x0, dy, img_w, img_h);
    else
        mat->Draw(x0, dy, V_SCALE);
}

//===========================================================================
//  ListWidget
//===========================================================================

void ListWidget::SelectByValue(int v)
{
    for (int i = 0; i < static_cast<int>(entries.size()); i++)
    {
        if (entries[i].value == v)
        {
            selected = i;
            // Scroll to make it visible.
            if (selected < scroll_top)
                scroll_top = selected;
            else if (selected >= scroll_top + visible_rows)
                scroll_top = selected - visible_rows + 1;
            return;
        }
    }
}

void ListWidget::Draw(const MenuDrawCtx &ctx, int dy, bool focused)
{
    int x0    = ctx.panel_x + ctx.panel_padding;
    int total = static_cast<int>(entries.size());

    // Clamp scroll so selected stays visible.
    if (selected < scroll_top)
        scroll_top = selected;
    if (selected >= scroll_top + visible_rows)
        scroll_top = selected - visible_rows + 1;
    if (scroll_top < 0)
        scroll_top = 0;
    if (scroll_top + visible_rows > total)
        scroll_top = total - visible_rows;
    if (scroll_top < 0)
        scroll_top = 0;

    for (int r = 0; r < visible_rows; r++)
    {
        int idx  = scroll_top + r;
        int row_y = dy + r * ctx.row_height;

        if (idx >= total)
            break;

        bool row_sel = (idx == selected);

        if (row_sel)
        {
            // Focused+selected: bright highlight. Unfocused+selected: dim.
            if (focused)
                ctx.mat_focus->DrawFill(x0 - 2, row_y - 1,
                                        ctx.panel_w - ctx.panel_padding * 2 + 4,
                                        ctx.row_height - 1);
            else
                ctx.mat_header->DrawFill(x0 - 2, row_y - 1,
                                         ctx.panel_w - ctx.panel_padding * 2 + 4,
                                         ctx.row_height - 1);
        }

        int flags = row_sel ? (V_WHITEMAP | V_SCALE) : V_SCALE;
        if (row_sel && focused)
            current_colormap = whitemap;

        ctx.font->DrawString(x0, row_y, entries[idx].label.c_str(), flags);
    }

    // Scroll indicator on the right edge.
    if (total > visible_rows)
    {
        int bar_x = ctx.panel_x + ctx.panel_w - ctx.panel_padding - 4;
        int bar_h = Height(ctx.row_height, ctx.header_height);
        int thumb_h = bar_h * visible_rows / total;
        if (thumb_h < 4) thumb_h = 4;
        int thumb_y = dy + bar_h * scroll_top / total;
        ctx.mat_header->DrawFill(bar_x, dy, 3, bar_h);
        ctx.mat_focus->DrawFill(bar_x, thumb_y, 3, thumb_h);
    }
}

bool ListWidget::HandleKey(int key)
{
    int total = static_cast<int>(entries.size());
    if (total == 0)
        return false;

    if (key == KEY_UPARROW || key == KEY_LEFTARROW)
    {
        if (selected <= 0)
            return false; // at top — let WidgetPanel navigate away
        selected--;
        if (on_select)
            on_select(entries[selected].value, userdata);
        return true;
    }

    if (key == KEY_DOWNARROW || key == KEY_RIGHTARROW)
    {
        if (selected >= total - 1)
            return false; // at bottom — let WidgetPanel navigate away
        selected++;
        if (on_select)
            on_select(entries[selected].value, userdata);
        return true;
    }

    if (key == KEY_ENTER)
    {
        if (on_select)
            on_select(entries[selected].value, userdata);
        return true;
    }

    return false;
}
