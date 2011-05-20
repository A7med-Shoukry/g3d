/**
  \file MemoryManager.cpp

  \created 2011-05-16
  \edited  2011-05-16

  Copyright 2000-2011, Morgan McGuire.
  All rights reserved.
 */

#include "G3D/PoolMemoryManager.h"
#include "G3D/GMutex.h"
#include "G3D/System.h"

namespace G3D {

#define REALPTR_TO_USERPTR(x)   ((uint8*)(x) + sizeof(uint32))
#define USERPTR_TO_REALPTR(x)   ((uint8*)(x) - sizeof(uint32))
#define USERSIZE_TO_REALSIZE(x)       ((x) + sizeof(uint32))
#define REALSIZE_FROM_USERPTR(u) (*(uint32*)USERPTR_TO_REALPTR(ptr) + sizeof(uint32))
#define USERSIZE_FROM_USERPTR(u) (*(uint32*)USERPTR_TO_REALPTR(ptr))

class BufferPool {
public:

    /** Only store buffers up to these sizes (in bytes) in each pool->
        Different pools have different management strategies.

        A large block is preallocated for tiny buffers; they are used with
        tremendous frequency.  Other buffers are allocated as demanded.
        Tiny buffers are 128 bytes long because that seems to align well with
        cache sizes on many machines.
      */
    enum {tinyBufferSize = 128, smallBufferSize = 1024, medBufferSize = 4096};

    /** 
       Most buffers we're allowed to store.
       250000 * 128  = 32 MB (preallocated)
        10000 * 1024 = 10 MB (allocated on demand)
         1024 * 4096 =  4 MB (allocated on demand)
     */
    enum {maxTinyBuffers = 250000, maxSmallBuffers = 10000, maxMedBuffers = 1024};

private:

    /** Pointer given to the program.  Unless in the tiny heap, the user size of the block is stored right in front of the pointer as a uint32.*/
    typedef void* UserPtr;

    /** Actual block allocated on the heap */
    typedef void* RealPtr;

    class MemBlock {
    public:
        UserPtr     ptr;
        size_t      bytes;

        inline MemBlock() : ptr(NULL), bytes(0) {}
        inline MemBlock(UserPtr p, size_t b) : ptr(p), bytes(b) {}
    };

    MemBlock smallPool[maxSmallBuffers];
    int smallPoolSize;

    MemBlock medPool[maxMedBuffers];
    int medPoolSize;

    /** The tiny pool is a single block of storage into which all tiny
        objects are allocated.  This provides better locality for
        small objects and avoids the search time, since all tiny
        blocks are exactly the same size. */
    void* tinyPool[maxTinyBuffers];
    int tinyPoolSize;

    /** Pointer to the data in the tiny pool */
    void* tinyHeap;

    Spinlock            m_lock;

    void lock() {
        m_lock.lock();
    }

    void unlock() {
        m_lock.unlock();
    }

    /** 
     Malloc out of the tiny heap. Returns NULL if allocation failed.
     */
    inline UserPtr tinyMalloc(size_t bytes) {
        // Note that we ignore the actual byte size
        // and create a constant size block.
        (void)bytes;
        debugAssert(tinyBufferSize >= bytes);

        UserPtr ptr = NULL;

        if (tinyPoolSize > 0) {
            --tinyPoolSize;

            // Return the old last pointer from the freelist
            ptr = tinyPool[tinyPoolSize];

#           ifdef G3D_DEBUG
                if (tinyPoolSize > 0) {
                    debugAssert(tinyPool[tinyPoolSize - 1] != ptr); 
                     //   "System::memoryManager()->alloc heap corruption detected: "
                     //   "the last two pointers on the freelist are identical (during tinyMalloc).");
                }
#           endif

            // NULL out the entry to help detect corruption
            tinyPool[tinyPoolSize] = NULL;
        }

        return ptr;
    }

    /** Returns true if this is a pointer into the tiny heap. */
    bool inTinyHeap(UserPtr ptr) {
        return 
            (ptr >= tinyHeap) && 
            (ptr < (uint8*)tinyHeap + maxTinyBuffers * tinyBufferSize);
    }

    void tinyFree(UserPtr ptr) {
        debugAssert(ptr);
        debugAssert(tinyPoolSize < maxTinyBuffers);
 //           "Tried to free a tiny pool buffer when the tiny pool freelist is full.");

#       ifdef G3D_DEBUG
            if (tinyPoolSize > 0) {
                UserPtr prevOnHeap = tinyPool[tinyPoolSize - 1];
                debugAssert(prevOnHeap != ptr); 
//                    "System::memoryManager()->alloc heap corruption detected: "
//                    "the last two pointers on the freelist are identical (during tinyFree).");
            }
#       endif

        debugAssert(tinyPool[tinyPoolSize] == NULL);

        // Put the pointer back into the free list
        tinyPool[tinyPoolSize] = ptr;
        ++tinyPoolSize;

    }

    void flushPool(MemBlock* pool, int& poolSize) {
        for (int i = 0; i < poolSize; ++i) {
            bytesAllocated -= USERSIZE_TO_REALSIZE(pool[i].bytes);
            ::free(USERPTR_TO_REALPTR(pool[i].ptr));
            pool[i].ptr = NULL;
            pool[i].bytes = 0;
        }
        poolSize = 0;
    }


    /** Allocate out of a specific pool.  Return NULL if no suitable 
        memory was found. */
    UserPtr malloc(MemBlock* pool, int& poolSize, size_t bytes) {

        // OPT: find the smallest block that satisfies the request.

        // See if there's something we can use in the buffer pool.
        // Search backwards since usually we'll re-use the last one.
        for (int i = (int)poolSize - 1; i >= 0; --i) {
            if (pool[i].bytes >= bytes) {
                // We found a suitable entry in the pool.

                // No need to offset the pointer; it is already offset
                UserPtr ptr = pool[i].ptr;

                // Remove this element from the pool, replacing it with
                // the one from the end (same as Array::fastRemove)
                --poolSize;
                pool[i] = pool[poolSize];

                return ptr;
            }
        }

        return NULL;
    }

public:

    /** Count of memory allocations that have occurred. */
    int totalMallocs;
    int mallocsFromTinyPool;
    int mallocsFromSmallPool;
    int mallocsFromMedPool;

    /** Amount of memory currently allocated (according to the application). 
        This does not count the memory still remaining in the buffer pool,
        but does count extra memory required for rounding off to the size
        of a buffer.
        Primarily useful for detecting leaks.*/
    // TODO: make me an atomic int!
    volatile int bytesAllocated;

    BufferPool() {
        totalMallocs         = 0;

        mallocsFromTinyPool  = 0;
        mallocsFromSmallPool = 0;
        mallocsFromMedPool   = 0;

        bytesAllocated       = true;

        tinyPoolSize         = 0;
        tinyHeap             = NULL;

        smallPoolSize        = 0;

        medPoolSize          = 0;


        // Initialize the tiny heap as a bunch of pointers into one
        // pre-allocated buffer.
        tinyHeap = ::malloc(maxTinyBuffers * tinyBufferSize);
        for (int i = 0; i < maxTinyBuffers; ++i) {
            tinyPool[i] = (uint8*)tinyHeap + (tinyBufferSize * i);
        }
        tinyPoolSize = maxTinyBuffers;
    }


    ~BufferPool() {
        ::free(tinyHeap);
        flushPool(smallPool, smallPoolSize);
        flushPool(medPool, medPoolSize);
    }

    
    UserPtr realloc(UserPtr ptr, size_t bytes) {
        if (ptr == NULL) {
            return malloc(bytes);
        }

        if (inTinyHeap(ptr)) {
            if (bytes <= tinyBufferSize) {
                // The old pointer actually had enough space.
                return ptr;
            } else {
                // Free the old pointer and malloc
                
                UserPtr newPtr = malloc(bytes);
                System::memcpy(newPtr, ptr, tinyBufferSize);
                tinyFree(ptr);
                return newPtr;

            }
        } else {
            // In one of our heaps.

            // See how big the block really was
            size_t userSize = USERSIZE_FROM_USERPTR(ptr);
            if (bytes <= userSize) {
                // The old block was big enough.
                return ptr;
            }

            // Need to reallocate and move
            UserPtr newPtr = malloc(bytes);
            System::memcpy(newPtr, ptr, userSize);
            free(ptr);
            return newPtr;
        }
    }


    UserPtr malloc(size_t bytes) {
        lock();
        ++totalMallocs;

        if (bytes <= tinyBufferSize) {

            UserPtr ptr = tinyMalloc(bytes);

            if (ptr) {
                ++mallocsFromTinyPool;
                unlock();
                return ptr;
            }

        } 
        
        // Failure to allocate a tiny buffer is allowed to flow
        // through to a small buffer
        if (bytes <= smallBufferSize) {
            
            UserPtr ptr = malloc(smallPool, smallPoolSize, bytes);

            if (ptr) {
                ++mallocsFromSmallPool;
                unlock();
                return ptr;
            }

        } else if (bytes <= medBufferSize) {
            // Note that a small allocation failure does *not* fall
            // through into a medium allocation because that would
            // waste the medium buffer's resources.

            UserPtr ptr = malloc(medPool, medPoolSize, bytes);

            if (ptr) {
                ++mallocsFromMedPool;
                unlock();
                debugAssertM(ptr != NULL, "BufferPool::malloc returned NULL");
                return ptr;
            }
        }

        bytesAllocated += USERSIZE_TO_REALSIZE(bytes);
        unlock();

        // Heap allocate

        // Allocate 4 extra bytes for our size header (unfortunate,
        // since malloc already added its own header).
        RealPtr ptr = ::malloc(USERSIZE_TO_REALSIZE(bytes));

        if (ptr == NULL) {
            // Flush memory pools to try and recover space
            flushPool(smallPool, smallPoolSize);
            flushPool(medPool, medPoolSize);
            ptr = ::malloc(USERSIZE_TO_REALSIZE(bytes));
        }

        if (ptr == NULL) {
            if ((System::outOfMemoryCallback() != NULL) &&
                (System::outOfMemoryCallback()(USERSIZE_TO_REALSIZE(bytes), true) == true)) {
                // Re-attempt the malloc
                ptr = ::malloc(USERSIZE_TO_REALSIZE(bytes));
            }
        }

        if (ptr == NULL) {
            if (System::outOfMemoryCallback() != NULL) {
                // Notify the application
                System::outOfMemoryCallback()(USERSIZE_TO_REALSIZE(bytes), false);
            }
#           ifdef G3D_DEBUG
            debugPrintf("::malloc(%d) returned NULL\n", (int)USERSIZE_TO_REALSIZE(bytes));
#           endif
            debugAssertM(ptr != NULL, 
                         "::malloc returned NULL. Either the "
                         "operating system is out of memory or the "
                         "heap is corrupt.");
            return NULL;
        }

        *(uint32*)ptr = bytes;

        return REALPTR_TO_USERPTR(ptr);
    }


    void free(UserPtr ptr) {
        if (ptr == NULL) {
            // Free does nothing on null pointers
            return;
        }

        debugAssert(isValidPointer(ptr));

        if (inTinyHeap(ptr)) {
            lock();
            tinyFree(ptr);
            unlock();
            return;
        }

        uint32 bytes = USERSIZE_FROM_USERPTR(ptr);

        lock();
        if (bytes <= smallBufferSize) {
            if (smallPoolSize < maxSmallBuffers) {
                smallPool[smallPoolSize] = MemBlock(ptr, bytes);
                ++smallPoolSize;
                unlock();
                return;
            }
        } else if (bytes <= medBufferSize) {
            if (medPoolSize < maxMedBuffers) {
                medPool[medPoolSize] = MemBlock(ptr, bytes);
                ++medPoolSize;
                unlock();
                return;
            }
        }
        bytesAllocated -= USERSIZE_TO_REALSIZE(bytes);
        unlock();

        // Free; the buffer pools are full or this is too big to store.
        ::free(USERPTR_TO_REALPTR(ptr));
    }

    std::string performance() const {
        if (totalMallocs > 0) {
            int pooled = mallocsFromTinyPool +
                         mallocsFromSmallPool + 
                         mallocsFromMedPool;

            int total = totalMallocs;

            return format("malloc performance: %5.1f%% <= %db, %5.1f%% <= %db, "
                          "%5.1f%% <= %db, %5.1f%% > %db",
                          100.0 * mallocsFromTinyPool  / total,
                          BufferPool::tinyBufferSize,
                          100.0 * mallocsFromSmallPool / total,
                          BufferPool::smallBufferSize,
                          100.0 * mallocsFromMedPool   / total,
                          BufferPool::medBufferSize,
                          100.0 * (1.0 - (double)pooled / total),
                          BufferPool::medBufferSize);
        } else {
            return "No System::memoryManager()->alloc calls made yet.";
        }
    }

    std::string status() const {
        return format("preallocated shared buffers: %5d/%d x %db",
            maxTinyBuffers - tinyPoolSize, maxTinyBuffers, tinyBufferSize);
    }
};

////////////////////////////////////////////////////////////////

PoolMemoryManager::PoolMemoryManager() {
    m_bufferPool = new BufferPool();
}

PoolMemoryManager::~PoolMemoryManager() {
    delete m_bufferPool;
}

void* PoolMemoryManager::alloc(size_t s) {
    return m_bufferPool->malloc(s);
}


void PoolMemoryManager::free(void* ptr) {
    m_bufferPool->free(ptr);
}

std::string PoolMemoryManager::describePerformance() const {
    return m_bufferPool->performance();
}

void PoolMemoryManager::resetPerformance() {
    m_bufferPool->totalMallocs         = 0;
    m_bufferPool->mallocsFromMedPool   = 0;
    m_bufferPool->mallocsFromSmallPool = 0;
    m_bufferPool->mallocsFromTinyPool  = 0;
}


} // namespace G3D
