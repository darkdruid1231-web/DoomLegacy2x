// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: z_ascache.cpp $
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
/// \brief Async cache loader implementation.

#include "z_ascache.h"

#include "doomdef.h"
#include "command.h"
#include "console.h"
#include "g_game.h"

#include "w_wad.h"
#include "r_data.h"
#include "r_sprite.h"
#include "hardware/md3.h"
#include "s_sound.h"
#include "sounds.h"

#include <vector>
#include <string>


// Console variables for async loading
static CV_PossibleValue_t cv_async_onoff_values[] = { {0, "Off"}, {1, "On"}, {0, NULL} };
static CV_PossibleValue_t CV_AsyncMaxPending[] = { {4, "MIN"}, {64, "MAX"}, {0, NULL} };

// Global console variables - declared in header for access
consvar_t cv_async_loading = { "async_loading", "1", CV_SAVE, cv_async_onoff_values, NULL, 1 };
consvar_t cv_async_maxpending = { "async_maxpending", "32", CV_SAVE, CV_AsyncMaxPending, NULL, 32 };

//=========================================================================
// Completion tracking
//=========================================================================

static std::vector<LoadCompletion> g_completions;
static Mutex g_completionMutex;

void AsyncCache_AddCompletion(const LoadCompletion &completion)
{
    MutexLocker lock(g_completionMutex);
    g_completions.push_back(completion);
}

void AsyncCache_ProcessCompletions()
{
    std::vector<LoadCompletion> completions;
    
    // Swap out completions under lock
    {
        MutexLocker lock(g_completionMutex);
        completions.swap(g_completions);
    }
    
    // Process completions (outside lock to avoid deadlock)
    // Currently just clears the vector - items are already in cache
    // Future: could call registered callbacks here
    
    // Note: Resources are now in the cache and available for synchronous access
}

int AsyncCache_GetCompletionCount()
{
    MutexLocker lock(g_completionMutex);
    return (int)g_completions.size();
}

//=========================================================================
// AsyncCacheLoader Implementation
//=========================================================================

// Static instance pointer
static AsyncCacheLoader *g_asyncLoader = NULL;

AsyncCacheLoader &AsyncCacheLoader::Instance()
{
    if (!g_asyncLoader)
        g_asyncLoader = new AsyncCacheLoader();
    return *g_asyncLoader;
}

AsyncCacheLoader::AsyncCacheLoader()
    : running(false), maxPending(32)
{
}

AsyncCacheLoader::~AsyncCacheLoader()
{
    Stop();
}

bool AsyncCacheLoader::Start()
{
    if (running)
        return true;

    // Register console variables
    cv_async_loading.Reg();
    cv_async_maxpending.Reg();

    // Start the loader thread
    running = loaderThread.Start(LoaderThreadFunc, this, "AsyncLoader");
    if (!running)
    {
        CONS_Printf("Failed to start async loader thread\n");
        return false;
    }

    CONS_Printf("Async loader thread started\n");
    return true;
}

void AsyncCacheLoader::Stop()
{
    if (!running)
        return;

    running = false;
    
    // Signal the thread to stop by pushing a special request
    LoadRequest stopReq;
    stopReq.type = LOAD_TEXTURE;
    stopReq.name = "__STOP__";
    stopReq.priority = PRIORITY_CRITICAL;
    requestQueue.Push(stopReq);

    // Wait for thread to finish
    loaderThread.Join();
    
    CONS_Printf("Async loader thread stopped\n");
}

bool AsyncCacheLoader::QueueRequest(const LoadRequest &request)
{
    if (!running || !IsEnabled())
        return false;

    // Check queue limits
    if (GetPendingCount() >= cv_async_maxpending.value)
        return false;

    // Don't queue if already in progress
    processingMutex.Lock();
    for (size_t i = 0; i < processing.size(); i++)
    {
        if (processing[i].request.name == request.name)
        {
            processingMutex.Unlock();
            return false;
        }
    }
    processingMutex.Unlock();

    requestQueue.Push(request);
    return true;
}

bool AsyncCacheLoader::QueueTexture(const char *name, int priority)
{
    LoadRequest request(LOAD_TEXTURE, name, priority, NULL);
    return QueueRequest(request);
}

bool AsyncCacheLoader::QueueSound(const char *name, int priority)
{
    LoadRequest request(LOAD_SOUND, name, priority, NULL);
    return QueueRequest(request);
}

bool AsyncCacheLoader::QueueMaterial(const char *name, int priority)
{
    LoadRequest request(LOAD_MATERIAL, name, priority, NULL);
    return QueueRequest(request);
}

bool AsyncCacheLoader::QueueSprite(const char *name, int priority)
{
    LoadRequest request(LOAD_SPRITE, name, priority, NULL);
    return QueueRequest(request);
}

bool AsyncCacheLoader::QueueModel(const char *name, int priority)
{
    LoadRequest request(LOAD_MODEL, name, priority, NULL);
    return QueueRequest(request);
}

void AsyncCacheLoader::ProcessCompletions()
{
    // This is called from the main thread to process completed loads
    // Call the global function which handles completions
    AsyncCache_ProcessCompletions();
}

int AsyncCacheLoader::LoaderThreadFunc(void *data)
{
    AsyncCacheLoader *loader = static_cast<AsyncCacheLoader *>(data);
    
    CONS_Printf("Async loader thread running\n");
    
    while (loader->running)
    {
        LoadRequest request;
        
        // Wait for a request with timeout
        if (!loader->requestQueue.Pop(request, 1000))  // 1 second timeout
            continue;
        
        // Check for stop signal
        if (request.name == "__STOP__")
            break;
        
        // Add to processing list
        {
            MutexLocker lock(loader->processingMutex);
            loader->processing.push_back(PendingRequest(request, game.tic));
        }
        
        // Process the request
        cacheitem_t *item = loader->ProcessRequest(request);
        
        // Remove from processing list
        loader->RemoveProcessing(request);
        
        // Add completion for main thread processing
        LoadCompletion completion;
        completion.type = request.type;
        completion.name = request.name;
        completion.item = item;
        completion.success = (item != NULL);
        completion.userData = request.userData;
        
        AsyncCache_AddCompletion(completion);
        
        // Item is now in the cache, ready for main thread use
    }
    
    CONS_Printf("Async loader thread exiting\n");
    return 0;
}

cacheitem_t *AsyncCacheLoader::ProcessRequest(const LoadRequest &request)
{
    // Note: This runs in the background thread!
    // Must be thread-safe with WAD file access
    
    switch (request.type)
    {
        case LOAD_TEXTURE:
        {
            // Textures are cached via texture_cache_t
            // Use synchronous loading in the cache - it's thread-safe for reading
            Texture *tex = textures.Get(request.name.c_str());
            return tex;
        }
        
        case LOAD_MATERIAL:
        {
            // Materials are cached via material_cache_t
            // Get with normal priority
            Material *mat = materials.Get(request.name.c_str(), TEX_wall);
            return mat;
        }
        
        case LOAD_SOUND:
        {
            // Route through public helper to avoid needing the full soundcache_t definition
            sounditem_t *snd = S_PrecacheSound(request.name.c_str());
            return snd;
        }
        
        case LOAD_SPRITE:
        {
            // Sprites via spritecache_t
            // Load sprite - returns sprite_t* which inherits from cacheitem_t
            sprite_t *spr = sprites.Get(request.name.c_str());
            return spr;
        }
        
        case LOAD_MODEL:
        {
            // Models via modelcache_t
            // Load MD3 model - returns MD3_player* which inherits from cacheitem_t
            MD3_player *mdl = models.Get(request.name.c_str());
            return mdl;
        }
        
        default:
            return NULL;
    }
}

void AsyncCacheLoader::RemoveProcessing(const LoadRequest &request)
{
    MutexLocker lock(processingMutex);
    
    for (std::vector<PendingRequest>::iterator it = processing.begin(); 
         it != processing.end(); ++it)
    {
        if (it->request.name == request.name)
        {
            processing.erase(it);
            return;
        }
    }
}

void AsyncCacheLoader::Shutdown()
{
    if (g_asyncLoader)
    {
        delete g_asyncLoader;
        g_asyncLoader = NULL;
    }
}

//=========================================================================
// Helper Functions
//=========================================================================

/// Initialize async loading at startup
void AsyncCache_Init()
{
    AsyncCacheLoader::Instance().Start();
}

/// Shutdown async loading at exit
void AsyncCache_Shutdown()
{
    AsyncCacheLoader::Shutdown();
}

/// Queue a texture for async loading
void AsyncCache_QueueTexture(const char *name, int priority)
{
    AsyncCacheLoader::Instance().QueueTexture(name, priority);
}

/// Queue a sound for async loading  
void AsyncCache_QueueSound(const char *name, int priority)
{
    AsyncCacheLoader::Instance().QueueSound(name, priority);
}

/// Queue a material for async loading
void AsyncCache_QueueMaterial(const char *name, int priority)
{
    AsyncCacheLoader::Instance().QueueMaterial(name, priority);
}

/// Queue a sprite for async loading
void AsyncCache_QueueSprite(const char *name, int priority)
{
    AsyncCacheLoader::Instance().QueueSprite(name, priority);
}

/// Queue a model for async loading
void AsyncCache_QueueModel(const char *name, int priority)
{
    AsyncCacheLoader::Instance().QueueModel(name, priority);
}
