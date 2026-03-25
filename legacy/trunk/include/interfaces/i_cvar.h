//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Doom Legacy Team.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Console variable (cvar) interface for dependency injection.
///
/// This interface abstracts console variable access to enable mock-based
/// unit testing. Production code uses CVarProviderAdapter which delegates
/// to the real console variable system.

#ifndef INCLUDED_INTERFACES_I_CVAR_H
#define INCLUDED_INTERFACES_I_CVAR_H

/// \brief Console variable provider interface.
///
/// Abstracts access to console variables (cvars) to enable dependency
/// injection and mocking in tests.
class ICVarProvider {
public:
    virtual ~ICVarProvider() = default;

    /// Get an integer cvar value by name.
    /// \param name Cvar name
    /// \return Cvar value, or 0 if not found
    virtual int getInt(const char* name) const = 0;

    /// Get a string cvar value by name.
    /// \param name Cvar name
    /// \return Cvar string value, or empty string if not found
    virtual const char* getString(const char* name) const = 0;

    /// Set an integer cvar value by name.
    /// \param name Cvar name
    /// \param value New value
    /// \return true if cvar was found and set, false otherwise
    virtual bool setInt(const char* name, int value) = 0;

    /// Check if a cvar exists.
    /// \param name Cvar name
    /// \return true if cvar exists
    virtual bool exists(const char* name) const = 0;
};

#endif // INCLUDED_INTERFACES_I_CVAR_H
