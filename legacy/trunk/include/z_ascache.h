// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: z_ascache.h $
//
// Copyright (C) 2024 by DooM Legacy Team.
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
/// \brief Async cache loader for background resource loading.

#ifndef z_ascache_h
#define z_ascache_h 1

// Forward declarations
struct consvar_t;
class cacheitem_t;

// game.tic is accessed via the global game object (defined in game/g_game.h)

#include <vector>
#include <string>
#include <cstddef>

// Include threading primitives (we need TSQueue template)
#include "z_thread.h"
#include "z_cache.h"
#include "command.h"  // For CV_PossibleValue_t and consvar_t

// Console variables - declared in z_ascache.cpp
extern consvar_t cv_async_loading;
extern consvar_t cv_async_maxpending;

// Helper functions declared in z_ascache.cpp
void AsyncCache_Init();
void AsyncCache_Shutdown();
void AsyncCache_QueueTexture(const char *name, int priority);
void AsyncCache_QueueSound(const char *name, int priority);
void AsyncCache_QueueMaterial(const char *name, int priority);
void AsyncCache_QueueSprite(const char *name, int priority);
void AsyncCache_QueueModel(const char *name, int priority);

//=========================================================================
// Load Request
//=========================================================================

/// Priority levels for load requests
enum LoadPriority
{
    PRIORITY_LOW = 0,      ///< Background/distant resources
    PRIORITY_NORMAL = 1,   ///< Normal gameplay resources
    PRIORITY_HIGH = 2,     ///< Urgent resources (nearby, visible)
    PRIORITY_CRITICAL = 3  ///< Critical resources (UI, current screen)
};

/// Type of resource to load
enum LoadType
{
    LOAD_TEXTURE,
    LOAD_MATERIAL,
    LOAD_SOUND,
    LOAD_SPRITE,
    LOAD_MODEL
};

/// A load request for the async loader
struct LoadRequest
{
    LoadType   type;       ///< Type of resource
    std::string name;       ///< Resource name
    int        priority;   ///< Priority level
    void      *userData;   ///< User data for callback

    /// Callback when load completes - returns loaded item or NULL
    typedef cacheitem_t *(*LoadCallback)(const char *name, void *userData);

    LoadCallback callback;

    LoadRequest() : type(LOAD_TEXTURE), priority(PRIORITY_NORMAL), userData(NULL), callback(NULL) {}
    LoadRequest(LoadType t, const std::string &n, int p, LoadCallback cb, void *ud = NULL)
        : type(t), name(n), priority(p), userData(ud), callback(cb) {}
};

/// Comparator for LoadRequest priority queue (higher priority value = dequeued first)
struct LoadRequestCompare
{
    bool operator()(const LoadRequest &a, const LoadRequest &b) const
    {
        return a.priority < b.priority;
    }
};

//=========================================================================
// Completion tracking
//=========================================================================

/// Completed load info - passed to callbacks
struct LoadCompletion
{
    LoadType     type;       ///< Type of resource
    std::string  name;       ///< Resource name
    cacheitem_t *item;       ///< The loaded item (may be NULL if failed)
    bool         success;    ///< Whether load succeeded
    void        *userData;   ///< User data from request

    LoadCompletion() : type(LOAD_TEXTURE), item(NULL), success(false), userData(NULL) {}
};

// Helper functions for completion handling
void AsyncCache_AddCompletion(const LoadCompletion &completion);
void AsyncCache_ProcessCompletions();
int AsyncCache_GetCompletionCount();

//=========================================================================
// Async Cache Loader
//=========================================================================

/// Singleton class that manages async resource loading
class AsyncCacheLoader
{
  protected:
    /// Pending request with metadata
    struct PendingRequest
    {
        LoadRequest request;
        int         queueTime;  ///< Game tic when queued

        PendingRequest() : queueTime(0) {}
        PendingRequest(const LoadRequest &req, int time)
            : request(req), queueTime(time) {}
    };

    TSPQueue<LoadRequest, LoadRequestCompare> requestQueue;  ///< Priority queue of pending load requests
    std::vector<PendingRequest> processing;   ///< Currently processing requests
    Mutex                 processingMutex;    ///< Mutex for processing list

    Thread     loaderThread;  ///< Background loader thread
    bool       running;       ///< Is the loader running
    int        maxPending;    ///< Maximum pending requests

  public:
    /// Get singleton instance
    static AsyncCacheLoader &Instance();

    /// Constructor
    AsyncCacheLoader();

    /// Destructor - shuts down the loader thread
    ~AsyncCacheLoader();

    /// Start the async loader thread
    bool Start();

    /// Stop the async loader thread
    void Stop();

    /// Queue a resource for async loading
    /// Returns true if queued, false if queue full or async disabled
    bool QueueRequest(const LoadRequest &request);

    /// Queue a texture for async loading
    bool QueueTexture(const char *name, int priority = PRIORITY_NORMAL);

    /// Queue a sound for async loading
    bool QueueSound(const char *name, int priority = PRIORITY_NORMAL);

    /// Queue a material for async loading
    bool QueueMaterial(const char *name, int priority = PRIORITY_NORMAL);

    /// Queue a sprite for async loading
    bool QueueSprite(const char *name, int priority = PRIORITY_NORMAL);

    /// Queue a model for async loading
    bool QueueModel(const char *name, int priority = PRIORITY_NORMAL);

    /// Process completed async loads (call from main thread)
    void ProcessCompletions();

    /// Check if async loading is enabled
    bool IsEnabled() const { return cv_async_loading.value != 0; }

    /// Get number of pending requests
    int GetPendingCount() const { return requestQueue.Size() + processing.size(); }

    /// Get maximum pending requests setting
    int GetMaxPending() const { return cv_async_maxpending.value; }

    /// Shutdown function
    static void Shutdown();

  protected:
    /// Main loader thread function
    static int LoaderThreadFunc(void *data);

    /// Process a single load request
    cacheitem_t *ProcessRequest(const LoadRequest &request);

    /// Remove a completed request from processing list
    void RemoveProcessing(const LoadRequest &request);
};

#endif
