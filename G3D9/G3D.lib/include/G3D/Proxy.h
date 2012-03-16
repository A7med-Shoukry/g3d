/**
 \file   G3D/Proxy.h
 \author Morgan McGuire, http://graphics.cs.williams.edu
 \date   2012-03-16
 \edited 2012-03-16
*/
#ifndef G3D_Proxy_h
#define G3D_Proxy_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"

namespace G3D {

/** 
  \brief Provides a level of indirection for accessing objects to allow computing them on
  demand or extending them with metadata without subclassing the object itself.

  See Material for an example.
*/
template<class T>
class Proxy : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer< Proxy<T> > Ref;

    /** Returns a pointer to a non-NULL Material. That material is always a MaterialProxy 
       as well, but any recursion must be handled within the implementation; that is, if there
       are multiple levels of proxies, this call resolves all of them. */
    virtual const ReferenceCountedPointer<T> resolve() const = 0;

    /** \copydoc resolve */
    virtual ReferenceCountedPointer<T> resolve() = 0;
};

} // namespace G3D

#endif // G3D_Proxy_h
