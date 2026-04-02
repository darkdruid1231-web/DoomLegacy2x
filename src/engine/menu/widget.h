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
/// \brief Widget-based UI component model for the modern menu renderer.
///
/// Phase 1: replaces DrawMenuModern()'s flat item loop with a proper object
/// hierarchy.  The classic DrawMenu() path and menuitem_t[] arrays are
/// untouched.  MenuToWidgetPanel() is defined in menu.cpp (where menuitem_t
/// is fully visible) and acts as the adapter between the two worlds.

#ifndef menu_widget_h
#define menu_widget_h 1

#include <string>
#include <vector>

#include "backends/IRenderBackend.h"

// Forward declarations — widget.h is self-contained; widget.cpp pulls in the
// concrete headers it needs.
class  font_t;
class  Material;
struct consvar_t;

//===========================================================================
//  Rendering context
//===========================================================================

/// All state that DrawMenuModern() passes to WidgetPanel::Draw().
/// Populated once per frame from menu.cpp's statics and Menu fields, then
/// forwarded to every Widget::Draw() call.
struct MenuDrawCtx
{
    // --- resources ----------------------------------------------------------
    IRenderBackend *backend;  ///< all menu drawing is routed through this backend
    font_t   *font;         ///< console font (may fall back to hud_font)
    Material *mat_panel;    ///< panel background material
    Material *mat_header;   ///< section-header background material
    Material *mat_focus;    ///< focused-row highlight material

    // --- panel layout (Doom virtual 320×200 units) --------------------------
    int panel_x, panel_y;  ///< top-left corner of the drawn panel
    int panel_w, panel_h;  ///< dimensions of the drawn panel
    int panel_padding;      ///< inner horizontal/vertical padding
    int row_height;         ///< height of a standard item row
    int header_height;      ///< height of a section-header row
    int value_box_w;        ///< width of the right-hand value/option box
    int slider_width;       ///< total rendered width of a slider bar

    // --- input / animation state --------------------------------------------
    short itemOn;           ///< currently focused item index (from Menu::itemOn)
    int   AnimCount;        ///< skull/cursor blink counter

    // --- dropdown overlay state (mirrors menu_dropdown_* statics) -----------
    bool  dropdown_open;
    int   dropdown_item;    ///< item index whose dropdown is open (-1 = none)
    int   dropdown_index;   ///< highlighted option inside the open dropdown

    // --- textbox state -------------------------------------------------------
    bool        textbox_active; ///< textbox.Active()
    const char *textbox_text;   ///< textbox.GetText() (valid when textbox_active)

    // --- footer --------------------------------------------------------------
    const char *disabled_reason; ///< tooltip shown below panel for disabled items
};

//===========================================================================
//  Widget ABC
//===========================================================================

/// Abstract base for all menu UI components.
/// Each concrete type handles one or more menuflag_t display/type combinations.
class Widget
{
  public:
    /// Extra pixels added above this row before Draw() is called.
    /// Set by MenuToWidgetPanel() for menuitem_t entries with IT_DY.
    int extra_top_margin = 0;

    /// When non-null, this widget is grayed and non-focusable whenever the
    /// named cvar has value == 0.  Checked every frame so live toggling works.
    consvar_t *enabled_if_cvar = nullptr;

    /// Returns true when enabled_if_cvar is set and currently off (== 0).
    /// Implemented in widget.cpp (consvar_t is incomplete here).
    bool IsConditionallyDisabled() const;

    /// Draw this widget's content into the panel.
    ///
    /// \param ctx     shared rendering context (layout, materials, focus state)
    /// \param dy      top-left y of this row in Doom virtual units
    /// \param focused true when itemOn == this widget's index in the panel
    virtual void Draw(const MenuDrawCtx &ctx, int dy, bool focused) = 0;

    /// Per-frame tick (animation etc.).
    virtual void Tick() {}

    /// Whether this widget can receive keyboard focus.
    virtual bool CanFocus() const { return false; }

    /// Total row height contributed by this widget.
    /// Default = row_height.  Multi-row widgets (ListWidget) override this.
    virtual int Height(int row_height, int header_height) const { return row_height; }

    /// True if this widget handles its own UP/DOWN navigation internally
    /// (e.g. ListWidget).  When true, MenuResponder will call HandleKey()
    /// directly for arrow keys instead of delegating to WidgetPanel::HandleNavKey().
    virtual bool HandlesNavKeys() const { return false; }

    /// Handle a key press directed at this widget (called when focused).
    /// Returns true if the key was consumed.
    virtual bool HandleKey(int /*key*/) { return false; }

    virtual ~Widget() = default;
};

//===========================================================================
//  Concrete widget types
//===========================================================================

/// Static label: IT_STRING / IT_CSTRING / plain-text IT_PATCH.
/// Also renders IT_HEADER_FLAG section headers.
class LabelWidget : public Widget
{
  public:
    const char *text;
    bool is_header; ///< draws header-chrome background, white text
    bool is_white;  ///< non-header label that uses the white colormap

    LabelWidget(const char *t, bool hdr, bool white)
        : text(t), is_header(hdr), is_white(white) {}

    void Draw(const MenuDrawCtx &ctx, int dy, bool focused) override;
    int  Height(int row_height, int header_height) const override
    {
        return is_header ? header_height : row_height;
    }
    // Labels are never focusable.
};

/// Focusable action button: IT_FUNC / IT_SUBMENU / IT_CONTROL.
/// Highlighted when focused; the actual action is still fired by MenuResponder.
class ButtonWidget : public Widget
{
  public:
    const char *text;
    bool        disabled;

    ButtonWidget(const char *t, bool dis) : text(t), disabled(dis) {}

    void Draw(const MenuDrawCtx &ctx, int dy, bool focused) override;
    bool CanFocus() const override { return !disabled && !IsConditionallyDisabled(); }
};

/// ConsVar toggle / enum with optional dropdown: IT_CV / IT_DROPDOWN.
/// Draws the label on the left and the current value box on the right.
class OptionWidget : public Widget
{
  public:
    const char *text;
    consvar_t  *cv;
    int         item_flags; ///< relevant menuflag_t bits (IT_DROPDOWN, IT_WHITE…)
    bool        disabled;

    OptionWidget(const char *t, consvar_t *c, int f, bool dis)
        : text(t), cv(c), item_flags(f), disabled(dis) {}

    void Draw(const MenuDrawCtx &ctx, int dy, bool focused) override;
    bool CanFocus() const override { return !disabled && !IsConditionallyDisabled(); }
};

/// ConsVar numeric slider bar: IT_CV_SLIDER / IT_CV_BIGSLIDER.
class SliderWidget : public Widget
{
  public:
    const char *text;
    consvar_t  *cv;
    bool        disabled;

    SliderWidget(const char *t, consvar_t *c, bool dis)
        : text(t), cv(c), disabled(dis) {}

    void Draw(const MenuDrawCtx &ctx, int dy, bool focused) override;
    bool CanFocus() const override { return !disabled && !IsConditionallyDisabled(); }
};

/// ConsVar text-entry field: IT_CV_TEXTBOX.
/// Delegates display of live input to the textbox state in MenuDrawCtx.
class TextInputWidget : public Widget
{
  public:
    const char *text;
    consvar_t  *cv;
    /// Live pointer into menuitem_t::flags so we can read IT_TEXTBOX_IN_USE.
    /// The menuitem_t outlives the WidgetPanel (both are module-lifetime data).
    short      *item_flags_ptr;

    TextInputWidget(const char *t, consvar_t *c, short *f)
        : text(t), cv(c), item_flags_ptr(f) {}

    void Draw(const MenuDrawCtx &ctx, int dy, bool focused) override;
    bool CanFocus() const override { return !IsConditionallyDisabled(); }
};

/// Vertical spacer or extra top-margin placeholder: IT_SPACE / IT_DY.
class SpaceWidget : public Widget
{
  public:
    void Draw(const MenuDrawCtx &ctx, int dy, bool focused) override {}
    // Not focusable.
};

//===========================================================================
//  Phase 2 widget types
//===========================================================================

/// Horizontal separator rule with optional label.
class SeparatorWidget : public Widget
{
  public:
    const char *label; ///< optional centred label (may be NULL)

    explicit SeparatorWidget(const char *l = nullptr) : label(l) {}

    void Draw(const MenuDrawCtx &ctx, int dy, bool focused) override;
    // Not focusable.
};

/// Inline image drawn from a named Material.
class ImageWidget : public Widget
{
  public:
    const char *tex_name; ///< Material name passed to materials.Get()
    int         img_w;    ///< drawn width in Doom virtual units (0 = native)
    int         img_h;    ///< drawn height in Doom virtual units (0 = native)

    ImageWidget(const char *name, int w = 0, int h = 0)
        : tex_name(name), img_w(w), img_h(h) {}

    void Draw(const MenuDrawCtx &ctx, int dy, bool focused) override;
    int  Height(int row_height, int /*header_height*/) const override
    {
        return img_h > 0 ? img_h : row_height;
    }
};

/// One entry in a ListWidget.
struct ListEntry
{
    std::string label; ///< display text
    int         value; ///< opaque integer stored with the entry (e.g. map number)

    ListEntry(const char *l, int v) : label(l), value(v) {}
    ListEntry(const std::string &l, int v) : label(l), value(v) {}
};

/// Callback type invoked when the user confirms a ListWidget selection.
typedef void (*ListSelectCallback)(int value, void *userdata);

/// Scrollable single-column list: Phase 2 replacement for repeated-cvar hacks.
///
/// Navigation:
///   UP / DOWN  — move selection; returns false at boundaries so focus
///                can pass to the previous / next widget.
///   ENTER      — fires on_select callback and stays focused.
///
/// Height = visible_rows * row_height.
class ListWidget : public Widget
{
  protected:
    std::vector<ListEntry> entries;
    int selected    = 0;  ///< currently highlighted row index
    int scroll_top  = 0;  ///< first visible row index
    int visible_rows;     ///< number of rows shown at once

    ListSelectCallback on_select = nullptr;
    void              *userdata  = nullptr;

  public:
    explicit ListWidget(int rows = 5) : visible_rows(rows) {}

    void SetEntries(std::vector<ListEntry> e) { entries = std::move(e); selected = 0; scroll_top = 0; }
    void AppendEntry(const char *label, int value) { entries.emplace_back(label, value); }
    void ClearEntries() { entries.clear(); selected = 0; scroll_top = 0; }

    void SetCallback(ListSelectCallback cb, void *ud) { on_select = cb; userdata = ud; }

    int  GetSelectedValue() const { return entries.empty() ? -1 : entries[selected].value; }
    int  GetSelectedIndex() const { return selected; }

    /// Force-select by value.  No-op if value not found.
    void SelectByValue(int v);

    void Draw(const MenuDrawCtx &ctx, int dy, bool focused) override;
    bool HandleKey(int key) override;
    bool CanFocus() const override { return !entries.empty(); }
    bool HandlesNavKeys() const override { return true; }
    int  Height(int row_height, int /*header_height*/) const override
    {
        return visible_rows * row_height;
    }
};

//===========================================================================
//  WidgetPanel container
//===========================================================================

/// Owns and lays out a list of Widgets for one menu.
///
/// Responsibilities taken from DrawMenuModern():
///   - panel background
///   - per-row focus highlight / header chrome
///   - calling Widget::Draw() in order
///   - dropdown overlay pass
///   - disabled-reason footer
///
/// MenuToWidgetPanel() (defined in menu.cpp) constructs and populates panels.
/// Menu::DrawMenuModern() builds / caches them.
class WidgetPanel
{
    std::vector<Widget *> widgets;

    /// Current scroll position in pixels from the top of content.
    /// Adjusted by EnsureVisible() each frame so the focused item is on-screen.
    int scroll_offset = 0;

    /// Pixel distance from the content top to the top of widget at index idx.
    int ItemTopY(int idx, int row_height, int header_height) const;

    /// Adjust scroll_offset so widget idx is fully visible within visible_h pixels.
    void EnsureVisible(int idx, int row_height, int header_height, int visible_h);

  public:
    WidgetPanel() {}
    ~WidgetPanel();

    void Add(Widget *w) { widgets.push_back(w); }

    int Count() const { return static_cast<int>(widgets.size()); }

    Widget *At(int i)
    {
        return (i >= 0 && i < Count()) ? widgets[i] : nullptr;
    }

    /// Draw the whole panel.  ctx must be fully populated by DrawMenuModern().
    /// Clips widget drawing to the panel rect and auto-scrolls to the focused item.
    void Draw(const MenuDrawCtx &ctx);

    /// Sum of all widget heights (extra_top_margin + Height()) for panel sizing.
    int ContentHeight(int row_height, int header_height) const;

    /// Handle UP / DOWN arrow focus navigation.
    /// Skips non-focusable widgets (CanFocus() == false).
    /// Returns true if the key was consumed (and itemOn was updated).
    /// Returns false for any other key so MenuResponder can handle it.
    /// NOTE: only call this when the focused widget does NOT have HandlesNavKeys()==true.
    bool HandleNavKey(int key, short &itemOn, int numitems);
};

#endif // menu_widget_h
