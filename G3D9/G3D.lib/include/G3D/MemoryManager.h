/**
  \file MemoryManager.h

  \created 2009-04-20
  \edited  2011-05-16

  Copyright 2000-2011, Morgan McGuire.
  All rights reserved.
 */
#ifndef G3D_MemoryManager_h
#define G3D_MemoryManager_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"

namespace G3D {

/** 
   Abstraction of memory management.
   Default implementation uses G3D::System::memoryManager()->alloc and is threadsafe.

   \sa LargePoolMemoryManager, CRTMemoryManager, AlignedMemoryManager, AreaMemoryManager */
class MemoryManager : public ReferenceCountedObject {
protected:

    MemoryManager();

public:

    typedef ReferenceCountedPointer<class MemoryManager> Ref;

    /** Return a pointer to \a s bytes of memory that are unused by
        the rest of the program.  The contents of the memory are
        undefined */
    virtual void* alloc(size_t s) = 0;

    /** Invoke to declare that this memory will no longer be used by
        the program.  The memory manager is not required to actually
        reuse or release this memory. */
    virtual void free(void* ptr) = 0;

    /** Returns true if this memory manager is threadsafe (i.e., alloc
        and free can be called asychronously) */
    virtual bool isThreadsafe() const = 0;

    /** If the allocation size of \a block can be changed to newBytes, it does so and returns true.  Otherwise, it returns false and the block allocation is unmodified. Growing may fail because there is no adjacent space.  Shrinking may fail because the memory was allocated out of a pool of fixed-size buffers.  Note that a failed shrinking can be ignored without impacting program behavior, but a failed growth must be resolved explicitly  Sample usage:
    <pre>
    If shrinking, perform your C++ cleanup on the objects that will be freed

    if (! mm->tryUpdateAllocSize(b, s) && (s > originalSize)) {
         void* n = mm->alloc(s);
         do your own C++ copy from b to n, if required, or simply System::memcpy if POD
         mm->free(b);
         b = n;
    }
    </pre>

    Compare to C realloc(), which is not safe for C++ objects or those with embedded references.
    */
    virtual bool tryUpdateAllocSize(void* block, size_t newBytes) { return false; }

    /// Allocates memory thrugh alloc then initializes all bytes to 0.
    virtual void* allocZeroed(size_t bytes);

    /// Returns implementation-specific string describing current allocation performance
    virtual std::string describePerformance() const { return "Performance N/A"; }

    /// Resets performance indicators used to create performance description
    virtual void resetPerformance() { }
};


/** 
   Allocates memory on 16-byte boundaries.
   \sa MemoryManager, CRTMemoryManager, AreaMemoryManager */
class AlignedMemoryManager : public MemoryManager {
protected:

    AlignedMemoryManager();

public:

    typedef ReferenceCountedPointer<class AlignedMemoryManager> Ref;

    
    virtual void* alloc(size_t s);

    virtual void free(void* ptr);

    virtual bool isThreadsafe() const;

    static AlignedMemoryManager::Ref create();
};


/** A MemoryManager implemented using the C runtime. Not recommended
    for general use; this is largely for debugging. */
class CRTMemoryManager : public MemoryManager {
protected:
    CRTMemoryManager();

public:
    typedef ReferenceCountedPointer<class MemoryManager> Ref;
    virtual void* alloc(size_t s);
    virtual void free(void* ptr);
    virtual bool isThreadsafe() const;

    /** There's only one instance of this memory manager; it is 
        cached after the first creation. */
    static CRTMemoryManager::Ref create();
};

} // namespace G3D

#endif // G3D_MemoryManager_h
