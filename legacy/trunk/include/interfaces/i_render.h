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
/// \brief Render backend interface for dependency injection in tests.
///
/// This interface abstracts access to map geometry (vertexes, segs, sectors, etc.)
/// to enable mock-based unit testing. Production code uses RenderBackendAdapter
/// which delegates to the real Rend class.

#ifndef INCLUDED_INTERFACES_I_RENDER_H
#define INCLUDED_INTERFACES_I_RENDER_H

// Forward declarations
struct vertex_t;
struct seg_t;
struct sector_t;
struct subsector_t;
struct node_t;
struct line_t;
struct side_t;

/// \brief Render backend interface.
///
/// Abstracts access to map geometry for rendering. This allows components
/// that need geometry data (e.g., line-of-sight calculations, pathfinding)
/// to be tested with mocks instead of requiring a full map.
class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;

    // --- Geometry counts ---

    virtual int getVertexCount() const = 0;
    virtual int getSegCount() const = 0;
    virtual int getSectorCount() const = 0;
    virtual int getSubsectorCount() const = 0;
    virtual int getNodeCount() const = 0;
    virtual int getLineCount() const = 0;

    // --- Geometry access ---

    virtual const vertex_t* getVertexes() const = 0;
    virtual const seg_t* getSegs() const = 0;
    virtual const sector_t* getSectors() const = 0;
    virtual const subsector_t* getSubsectors() const = 0;
    virtual const node_t* getNodes() const = 0;
    virtual const line_t* getLines() const = 0;
    virtual const side_t* getSides() const = 0;

    // --- Convenience ---

    /// Check if a subsector index is valid
    virtual bool isSubsectorValid(int index) const = 0;

    /// Check if a node index is valid
    virtual bool isNodeValid(int index) const = 0;
};

#endif // INCLUDED_INTERFACES_I_RENDER_H
