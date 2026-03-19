// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: z_thread.h $
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
/// \brief Threading primitives for async resource loading.

#ifndef z_thread_h
#define z_thread_h 1

#include "doomtype.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_mutex.h>
#include <SDL2/SDL_atomic.h>

//=========================================================================
// Thread
//=========================================================================

/// Simple thread wrapper around SDL_CreateThread
class Thread
{
  protected:
    SDL_Thread *thread;
    int         threadId;
    bool        running;

  public:
    /// Thread function signature
    typedef int (*ThreadFunc)(void *);

    Thread() : thread(NULL), threadId(0), running(false) {}
    virtual ~Thread() { Join(); }

    /// Start the thread with the given function and user data
    bool Start(ThreadFunc func, void *data, const char *name = "Thread")
    {
        if (running)
            return false;

        thread = SDL_CreateThread(func, name, data);
        if (!thread)
            return false;

        threadId = (int)SDL_GetThreadID(thread);
        running = true;
        return true;
    }

    /// Wait for thread to complete
    int Join()
    {
        if (!thread)
            return 0;

        int status;
        SDL_WaitThread(thread, &status);
        thread = NULL;
        running = false;
        return status;
    }

    /// Check if thread is running
    bool IsRunning() const { return running; }

    /// Get SDL thread ID
    int GetId() const { return threadId; }
};

//=========================================================================
// Mutex
//=========================================================================

/// Simple mutex wrapper around SDL_mutex
class Mutex
{
  protected:
    SDL_mutex *mutex;

  public:
    Mutex()
    {
        mutex = SDL_CreateMutex();
    }

    ~Mutex()
    {
        if (mutex)
            SDL_DestroyMutex(mutex);
    }

    /// Lock the mutex, blocks if already locked
    bool Lock()
    {
        return SDL_LockMutex(mutex) == 0;
    }

    /// Unlock the mutex
    bool Unlock()
    {
        return SDL_UnlockMutex(mutex) == 0;
    }

    /// Try to lock, returns immediately if already locked
    bool TryLock()
    {
        return SDL_TryLockMutex(mutex) == 0;
    }

    /// Get raw SDL mutex for advanced operations
    SDL_mutex *GetRaw() const { return mutex; }
};

//=========================================================================
// MutexLocker
//=========================================================================

/// RAII-style mutex locker
class MutexLocker
{
  protected:
    Mutex &m;
    bool   locked;

  public:
    explicit MutexLocker(Mutex &mutex) : m(mutex), locked(false)
    {
        m.Lock();
        locked = true;
    }

    ~MutexLocker()
    {
        if (locked)
            m.Unlock();
    }

    /// Release the lock early
    void Release()
    {
        if (locked)
        {
            m.Unlock();
            locked = false;
        }
    }
};

//=========================================================================
// Semaphore
//=========================================================================

/// Semaphore for producer/consumer pattern
class Semaphore
{
  protected:
    SDL_sem *sem;

  public:
    explicit Semaphore(int initial_count = 0)
    {
        sem = SDL_CreateSemaphore(initial_count);
    }

    ~Semaphore()
    {
        if (sem)
            SDL_DestroySemaphore(sem);
    }

    /// Decrement (wait), blocks if count is 0
    bool Wait(int timeout_ms = 0)
    {
        if (timeout_ms > 0)
            return SDL_SemWaitTimeout(sem, timeout_ms) == 0;
        return SDL_SemWait(sem) == 0;
    }

    /// Increment (post)
    bool Post()
    {
        return SDL_SemPost(sem) == 0;
    }

    /// Get current count
    int GetValue() const
    {
        return SDL_SemValue(sem);
    }
};

//=========================================================================
// Atomic Operations
//=========================================================================

/// Atomic counter for thread-safe reference counting
class AtomicCounter
{
  protected:
    SDL_atomic_t value;

  public:
    explicit AtomicCounter(int initial = 0)
    {
        SDL_AtomicSet(&value, initial);
    }

    /// Increment and return new value
    int Increment()
    {
        return SDL_AtomicIncRef(&value);
    }

    /// Decrement and return new value
    int Decrement()
    {
        return SDL_AtomicDecRef(&value);
    }

    /// Get current value
    int Get() const
    {
        return SDL_AtomicGet(const_cast<SDL_atomic_t*>(&value));
    }

    /// Set value
    void Set(int v)
    {
        SDL_AtomicSet(&value, v);
    }

    /// Compare and swap
    bool CAS(int oldval, int newval)
    {
        return SDL_AtomicCAS(&value, oldval, newval) != SDL_FALSE;
    }
};

//=========================================================================
// Thread-Safe Queue (Simple implementation)
//=========================================================================

/// Simple thread-safe queue using mutex and semaphore
template<typename T>
class TSQueue
{
  protected:
    struct Node
    {
        T data;
        Node *next;
        Node(const T &d) : data(d), next(NULL) {}
    };

    Node *head;
    Node *tail;
    int   count;
    Mutex mutex;
    Semaphore sem;

  public:
    TSQueue() : head(NULL), tail(NULL), count(0), sem(0) {}
    ~TSQueue()
    {
        Clear();
    }

    /// Push an item onto the queue
    void Push(const T &item)
    {
        Node *node = new Node(item);

        mutex.Lock();
        if (tail)
        {
            tail->next = node;
            tail = node;
        }
        else
        {
            head = tail = node;
        }
        count++;
        mutex.Unlock();

        sem.Post();
    }

    /// Pop an item from the queue (blocks if empty)
    bool Pop(T &item, int timeout_ms = 0)
    {
        if (!sem.Wait(timeout_ms))
            return false;

        mutex.Lock();
        Node *node = head;
        if (!node)
        {
            mutex.Unlock();
            return false;
        }

        item = node->data;
        head = node->next;
        if (!head)
            tail = NULL;
        count--;
        mutex.Unlock();

        delete node;
        return true;
    }

    /// Check if queue is empty
    bool IsEmpty() const
    {
        return count == 0;
    }

    /// Get number of items in queue
    int Size() const
    {
        return count;
    }

    /// Clear all items
    void Clear()
    {
        mutex.Lock();
        while (head)
        {
            Node *node = head;
            head = head->next;
            delete node;
        }
        tail = NULL;
        count = 0;
        mutex.Unlock();
    }
};

//=========================================================================
// Priority Thread-Safe Queue
//=========================================================================

/// Thread-safe queue with priority ordering (highest priority dequeued first).
/// Compare(a, b) returns true if a has lower priority than b (b should come first).
template<typename T, typename Compare>
class TSPQueue
{
  protected:
    struct Node
    {
        T data;
        Node *next;
        Node(const T &d) : data(d), next(NULL) {}
    };

    Node *head;
    int   count;
    Mutex mutex;
    Semaphore sem;
    Compare comp;

  public:
    TSPQueue() : head(NULL), count(0), sem(0) {}
    ~TSPQueue()
    {
        Clear();
    }

    /// Push an item, inserting at the correct priority position
    void Push(const T &item)
    {
        Node *node = new Node(item);

        mutex.Lock();
        if (!head || comp(head->data, item))
        {
            // item has higher (or equal) priority than current head — insert at front
            node->next = head;
            head = node;
        }
        else
        {
            Node *cur = head;
            while (cur->next && !comp(cur->next->data, item))
                cur = cur->next;
            node->next = cur->next;
            cur->next = node;
        }
        count++;
        mutex.Unlock();

        sem.Post();
    }

    /// Pop the highest-priority item (blocks if empty)
    bool Pop(T &item, int timeout_ms = 0)
    {
        if (!sem.Wait(timeout_ms))
            return false;

        mutex.Lock();
        Node *node = head;
        if (!node)
        {
            mutex.Unlock();
            return false;
        }

        item = node->data;
        head = node->next;
        count--;
        mutex.Unlock();

        delete node;
        return true;
    }

    /// Check if queue is empty
    bool IsEmpty() const
    {
        return count == 0;
    }

    /// Get number of items in queue
    int Size() const
    {
        return count;
    }

    /// Clear all items
    void Clear()
    {
        mutex.Lock();
        while (head)
        {
            Node *node = head;
            head = head->next;
            delete node;
        }
        count = 0;
        mutex.Unlock();
    }
};

#endif
