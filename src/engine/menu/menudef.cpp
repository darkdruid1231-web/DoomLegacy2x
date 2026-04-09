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
/// \brief MENUDEF lump parser implementation (hand-rolled recursive descent).

#include <cstring>
#include <cstdlib>
#include <cctype>
#include <map>
#include <string>

#include "doomdef.h"   // CONS_Printf
#include "command.h"   // consvar_t::FindVar
#include "w_wad.h"     // fc
#include "z_zone.h"    // PU_STATIC

#include "widget.h"
#include "menudef.h"

// ---------------------------------------------------------------------------
// Panel registry
// ---------------------------------------------------------------------------

static std::map<std::string, WidgetPanel *> s_menudef_panels;

WidgetPanel *MenuDef_FindPanel(const char *title)
{
    auto it = s_menudef_panels.find(title);
    return (it != s_menudef_panels.end()) ? it->second : nullptr;
}

void MenuDef_Clear()
{
    for (auto &kv : s_menudef_panels)
        delete kv.second;
    s_menudef_panels.clear();
}

// ---------------------------------------------------------------------------
// Tokeniser
// ---------------------------------------------------------------------------

struct Token
{
    enum Type { T_WORD, T_STRING, T_NUMBER, T_LBRACE, T_RBRACE, T_COMMA, T_EOF };
    Type        type;
    std::string str;   // T_WORD / T_STRING
    float       num;   // T_NUMBER
};

struct Tokeniser
{
    const char *src;
    const char *pos;
    int         line;
    const char *lump_name; // for error messages

    Tokeniser(const char *s, const char *name)
        : src(s), pos(s), line(1), lump_name(name) {}

    void SkipWhitespaceAndComments()
    {
        for (;;)
        {
            // skip blanks
            while (*pos && (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n'))
            {
                if (*pos == '\n') ++line;
                ++pos;
            }
            // line comment  // or #
            if (*pos == '/' && *(pos + 1) == '/')
            {
                while (*pos && *pos != '\n') ++pos;
                continue;
            }
            if (*pos == '#')
            {
                while (*pos && *pos != '\n') ++pos;
                continue;
            }
            // block comment /* ... */
            if (*pos == '/' && *(pos + 1) == '*')
            {
                pos += 2;
                while (*pos && !(*pos == '*' && *(pos + 1) == '/'))
                {
                    if (*pos == '\n') ++line;
                    ++pos;
                }
                if (*pos) pos += 2; // consume */
                continue;
            }
            break;
        }
    }

    Token Next()
    {
        SkipWhitespaceAndComments();
        Token t = {};
        if (!*pos) { t.type = Token::T_EOF; return t; }

        char c = *pos;

        if (c == '{') { ++pos; t.type = Token::T_LBRACE; return t; }
        if (c == '}') { ++pos; t.type = Token::T_RBRACE; return t; }
        if (c == ',') { ++pos; t.type = Token::T_COMMA;  return t; }

        // quoted string
        if (c == '"')
        {
            ++pos;
            t.type = Token::T_STRING;
            while (*pos && *pos != '"')
            {
                if (*pos == '\\' && *(pos + 1))
                {
                    ++pos;
                    switch (*pos)
                    {
                        case 'n':  t.str += '\n'; break;
                        case 't':  t.str += '\t'; break;
                        default:   t.str += *pos; break;
                    }
                }
                else
                    t.str += *pos;
                ++pos;
            }
            if (*pos == '"') ++pos;
            return t;
        }

        // number  (optional leading -)
        if (c == '-' || isdigit((unsigned char)c))
        {
            const char *start = pos;
            if (c == '-') ++pos;
            while (isdigit((unsigned char)*pos)) ++pos;
            if (*pos == '.') { ++pos; while (isdigit((unsigned char)*pos)) ++pos; }
            t.type = Token::T_NUMBER;
            t.num  = (float)atof(start);
            t.str  = std::string(start, pos - start);
            return t;
        }

        // word / keyword
        if (isalpha((unsigned char)c) || c == '_')
        {
            t.type = Token::T_WORD;
            while (isalnum((unsigned char)*pos) || *pos == '_') t.str += *pos++;
            return t;
        }

        // unknown — skip
        CONS_Printf("MENUDEF [%s] line %d: unexpected char '%c'\n", lump_name, line, c);
        ++pos;
        t.type = Token::T_EOF;
        return t;
    }

    // Peek without advancing
    Token Peek()
    {
        const char *saved = pos;
        int         saved_line = line;
        Token t = Next();
        pos  = saved;
        line = saved_line;
        return t;
    }

    // Expect a quoted string; returns "" on failure
    std::string ExpectString(const char *ctx)
    {
        Token t = Next();
        if (t.type != Token::T_STRING)
        {
            CONS_Printf("MENUDEF [%s] line %d: expected string after %s\n",
                        lump_name, line, ctx);
            return "";
        }
        return t.str;
    }

    // Expect a comma
    bool ExpectComma(const char *ctx)
    {
        Token t = Next();
        if (t.type != Token::T_COMMA)
        {
            CONS_Printf("MENUDEF [%s] line %d: expected ',' in %s\n",
                        lump_name, line, ctx);
            return false;
        }
        return true;
    }

    // Expect a number; returns 0 on failure
    float ExpectNumber(const char *ctx)
    {
        Token t = Next();
        if (t.type != Token::T_NUMBER)
        {
            CONS_Printf("MENUDEF [%s] line %d: expected number in %s\n",
                        lump_name, line, ctx);
            return 0.0f;
        }
        return t.num;
    }
};

// ---------------------------------------------------------------------------
// Case-insensitive keyword compare
// ---------------------------------------------------------------------------

static bool IEquals(const std::string &a, const char *b)
{
    size_t n = strlen(b);
    if (a.size() != n) return false;
    for (size_t i = 0; i < n; ++i)
        if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Parse one OptionMenu block  { ... }
// Returns nullptr on syntax error; caller takes ownership.
// ---------------------------------------------------------------------------

static WidgetPanel *ParseOptionMenu(Tokeniser &tok)
{
    // We already consumed "OptionMenu"; expect a title string then '{'
    std::string title = tok.ExpectString("OptionMenu");

    Token lbrace = tok.Next();
    if (lbrace.type != Token::T_LBRACE)
    {
        CONS_Printf("MENUDEF [%s] line %d: expected '{' after OptionMenu \"%s\"\n",
                    tok.lump_name, tok.line, title.c_str());
        return nullptr;
    }

    WidgetPanel *panel = new WidgetPanel();

    for (;;)
    {
        Token t = tok.Next();

        if (t.type == Token::T_RBRACE || t.type == Token::T_EOF)
            break;

        if (t.type != Token::T_WORD)
        {
            CONS_Printf("MENUDEF [%s] line %d: expected keyword, got '%s'\n",
                        tok.lump_name, tok.line, t.str.c_str());
            continue;
        }

        // --- Header "text" ---
        if (IEquals(t.str, "Header"))
        {
            std::string text = tok.ExpectString("Header");
            panel->Add(new LabelWidget(strdup(text.c_str()), /*hdr=*/true, /*white=*/false));
            continue;
        }

        // --- Separator  or  Separator "label" ---
        if (IEquals(t.str, "Separator"))
        {
            Token peek = tok.Peek();
            const char *lbl = nullptr;
            if (peek.type == Token::T_STRING)
            {
                tok.Next(); // consume
                lbl = strdup(peek.str.c_str());
            }
            panel->Add(new SeparatorWidget(lbl));
            continue;
        }

        // --- Space ---
        if (IEquals(t.str, "Space"))
        {
            panel->Add(new SpaceWidget());
            continue;
        }

        // --- Option "Label", "cvar_name" [enabled_if "cvar_name"] ---
        if (IEquals(t.str, "Option"))
        {
            std::string label = tok.ExpectString("Option");
            if (!tok.ExpectComma("Option")) { continue; }
            std::string cvname = tok.ExpectString("Option");

            consvar_t *cv = consvar_t::FindVar(cvname.c_str());
            if (!cv)
            {
                CONS_Printf("MENUDEF [%s] line %d: unknown cvar '%s'\n",
                            tok.lump_name, tok.line, cvname.c_str());
                continue;
            }
            OptionWidget *w = new OptionWidget(strdup(label.c_str()), cv, 0, false);
            Token ei = tok.Peek();
            if (ei.type == Token::T_WORD && IEquals(ei.str, "enabled_if"))
            {
                tok.Next();
                std::string ei_name = tok.ExpectString("enabled_if");
                consvar_t *ei_cv = consvar_t::FindVar(ei_name.c_str());
                if (!ei_cv)
                    CONS_Printf("MENUDEF [%s] line %d: enabled_if: unknown cvar '%s'\n",
                                tok.lump_name, tok.line, ei_name.c_str());
                else
                    w->enabled_if_cvar = ei_cv;
            }
            panel->Add(w);
            continue;
        }

        // --- Slider "Label", "cvar_name"[, min, max, step] [enabled_if "cvar_name"] ---
        if (IEquals(t.str, "Slider"))
        {
            std::string label = tok.ExpectString("Slider");
            if (!tok.ExpectComma("Slider")) { continue; }
            std::string cvname = tok.ExpectString("Slider");

            // optional numeric params — consume if present
            Token peek = tok.Peek();
            if (peek.type == Token::T_COMMA)
            {
                tok.Next(); // consume comma
                tok.ExpectNumber("Slider min");
                if (tok.Peek().type == Token::T_COMMA)
                {
                    tok.Next();
                    tok.ExpectNumber("Slider max");
                    if (tok.Peek().type == Token::T_COMMA)
                    {
                        tok.Next();
                        tok.ExpectNumber("Slider step");
                    }
                }
            }

            consvar_t *cv = consvar_t::FindVar(cvname.c_str());
            if (!cv)
            {
                CONS_Printf("MENUDEF [%s] line %d: unknown cvar '%s'\n",
                            tok.lump_name, tok.line, cvname.c_str());
                continue;
            }
            SliderWidget *w = new SliderWidget(strdup(label.c_str()), cv, false);
            Token ei = tok.Peek();
            if (ei.type == Token::T_WORD && IEquals(ei.str, "enabled_if"))
            {
                tok.Next();
                std::string ei_name = tok.ExpectString("enabled_if");
                consvar_t *ei_cv = consvar_t::FindVar(ei_name.c_str());
                if (!ei_cv)
                    CONS_Printf("MENUDEF [%s] line %d: enabled_if: unknown cvar '%s'\n",
                                tok.lump_name, tok.line, ei_name.c_str());
                else
                    w->enabled_if_cvar = ei_cv;
            }
            panel->Add(w);
            continue;
        }

        // --- Input "Label", "cvar_name" [enabled_if "cvar_name"] ---
        if (IEquals(t.str, "Input"))
        {
            std::string label = tok.ExpectString("Input");
            if (!tok.ExpectComma("Input")) { continue; }
            std::string cvname = tok.ExpectString("Input");

            consvar_t *cv = consvar_t::FindVar(cvname.c_str());
            if (!cv)
            {
                CONS_Printf("MENUDEF [%s] line %d: unknown cvar '%s'\n",
                            tok.lump_name, tok.line, cvname.c_str());
                continue;
            }
            TextInputWidget *w = new TextInputWidget(strdup(label.c_str()), cv, nullptr);
            Token ei = tok.Peek();
            if (ei.type == Token::T_WORD && IEquals(ei.str, "enabled_if"))
            {
                tok.Next();
                std::string ei_name = tok.ExpectString("enabled_if");
                consvar_t *ei_cv = consvar_t::FindVar(ei_name.c_str());
                if (!ei_cv)
                    CONS_Printf("MENUDEF [%s] line %d: enabled_if: unknown cvar '%s'\n",
                                tok.lump_name, tok.line, ei_name.c_str());
                else
                    w->enabled_if_cvar = ei_cv;
            }
            panel->Add(w);
            continue;
        }

        // --- Button "Label"[, "submenu_name"] [enabled_if "cvar_name"] ---
        if (IEquals(t.str, "Button") || IEquals(t.str, "SubMenu"))
        {
            std::string label = tok.ExpectString("Button/SubMenu");
            // optional second string argument (submenu name — ignored for now)
            if (tok.Peek().type == Token::T_COMMA)
            {
                tok.Next();
                tok.Next(); // consume the target string
            }
            ButtonWidget *w = new ButtonWidget(strdup(label.c_str()), false);
            Token ei = tok.Peek();
            if (ei.type == Token::T_WORD && IEquals(ei.str, "enabled_if"))
            {
                tok.Next();
                std::string ei_name = tok.ExpectString("enabled_if");
                consvar_t *ei_cv = consvar_t::FindVar(ei_name.c_str());
                if (!ei_cv)
                    CONS_Printf("MENUDEF [%s] line %d: enabled_if: unknown cvar '%s'\n",
                                tok.lump_name, tok.line, ei_name.c_str());
                else
                    w->enabled_if_cvar = ei_cv;
            }
            panel->Add(w);
            continue;
        }

        // --- Image "texname"  or  Image "texname", w, h ---
        if (IEquals(t.str, "Image"))
        {
            std::string texname = tok.ExpectString("Image");
            int img_w = 0, img_h = 0;
            if (tok.Peek().type == Token::T_COMMA)
            {
                tok.Next();
                img_w = (int)tok.ExpectNumber("Image width");
                if (tok.Peek().type == Token::T_COMMA)
                {
                    tok.Next();
                    img_h = (int)tok.ExpectNumber("Image height");
                }
            }
            panel->Add(new ImageWidget(strdup(texname.c_str()), img_w, img_h));
            continue;
        }

        CONS_Printf("MENUDEF [%s] line %d: unknown keyword '%s'\n",
                    tok.lump_name, tok.line, t.str.c_str());
    }

    // Register under the title (replace any existing panel with the same name)
    auto it = s_menudef_panels.find(title);
    if (it != s_menudef_panels.end())
    {
        delete it->second;
        it->second = panel;
    }
    else
    {
        s_menudef_panels[title] = panel;
    }

    return panel;
}

// ---------------------------------------------------------------------------
// Parse one lump
// ---------------------------------------------------------------------------

static void ParseMenuDefLump(const char *text, const char *lump_name)
{
    Tokeniser tok(text, lump_name);

    for (;;)
    {
        Token t = tok.Next();
        if (t.type == Token::T_EOF) break;

        if (t.type == Token::T_WORD && IEquals(t.str, "OptionMenu"))
        {
            ParseOptionMenu(tok);
            continue;
        }

        // Skip anything we don't recognise at top level
        if (t.type != Token::T_EOF)
            CONS_Printf("MENUDEF [%s] line %d: unexpected token '%s' at top level\n",
                        lump_name, tok.line, t.str.c_str());
    }
}

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

void MenuDef_LoadAll()
{
    // Iterate all lumps named MENUDEF in the WAD stack (last-loaded wins inside
    // each lump; later lumps override earlier ones via the register-replace logic).
    int lump = fc.FindNumForName("MENUDEF");
    if (lump < 0)
        return; // no MENUDEF lumps — nothing to do

    // Walk *all* matching lumps (fc.FindNumForName returns the last/topmost one;
    // iterate upward through the namespace to catch stacked WADs).
    // For simplicity we parse only the topmost MENUDEF — WAD stack ordering means
    // the topmost file's declarations win, which is the expected mod behaviour.
    const char *text = static_cast<const char *>(
        fc.CacheLumpNum(lump, PU_STATIC, /*add_NUL=*/true));

    ParseMenuDefLump(text, "MENUDEF");

    CONS_Printf("MENUDEF: loaded %u panel(s)\n",
                (unsigned)s_menudef_panels.size());
}
