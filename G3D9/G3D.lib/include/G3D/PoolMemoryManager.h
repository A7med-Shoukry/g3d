/**
  \file PoolMemoryManager.h

  \created 2011-05-16
  \edited  2011-05-16

  Copyright 2000-2011, Morgan McGuire.
  All rights reserved.
 */
#ifndef G3D_PoolMemoryManager_h
#define G3D_PoolMemoryManager_h

#include "G3D/MemoryManager.h"

namespace G3D {

class BufferPool;

/** \brief Implements a pooled allocator which returns
    allocations from pools of various sizes each with a 
    fixed number of available blocks.  Allocations greater
    than any pool size are allocated from system memory separately.

    \sa MemoryManager */
class PoolMemoryManager : public MemoryManager {
private:
    BufferPool* m_bufferPool;

public:
    PoolMemoryManager();
    virtual ~PoolMemoryManager();

    /// Allocates using System::memoryManager()->alloc
    virtual void* alloc(size_t numBytes);

    // Frees using System::memoryManager()->free
    virtual void free(void* ptr);

    /// System::memoryManager()->alloc and System::memoryManager()->free are thread-safe
    virtual bool isThreadsafe() const { return true; }

    /// Returns implementation-specific string describing current allocation performance
    virtual std::string describePerformance() const;

    /// Resets performance indicators used to create performance description
    virtual void resetPerformance();
};

} // namespace G3D

#endif // G3D_PoolMemoryManager_h
