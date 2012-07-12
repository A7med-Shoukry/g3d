/**
  \file GLG3D/VertexRange.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
  \created 2001-05-29
  \edited  2012-03-20
*/

#ifndef GLG3D_VertexRange_h
#define GLG3D_VertexRange_h

#include "GLG3D/getOpenGLState.h"
#include "GLG3D/glFormat.h"
#include "GLG3D/VertexBuffer.h"

namespace G3D {

class RenderDevice;
class Shader2;


/**
 \brief A block of GPU memory storing a stream of vector data (e.g.,
 vertices, normals, texture coordinates)
 
 A pointer to a "Vertex Arrray" of data (e.g., vertices,
 colors, or normals) in video memory.

 A VertexRange is just a pointer, so it is safe to copy these (the pointer
 will be copied, not the video memory).
 
 There is no destructor because the referenced memory is freed when
 the parent VertexBuffer is reset or freed.

 A VertexRange is normally a statically typed fixed-length array of a Vector
 or Color class, however it is possible to make a "void" array with the 
 constructor that does not take an array,
 and then fill it with data to create interleaved or structure arrays.
 Interleaved arrays are 2x - 3x as fast as non-interleaved ones for
 vertex-limited programs.
 */
class VertexRange {
private:

    friend class RenderDevice;
    friend class Shader2;

    VertexBuffer::Ref   m_area;
    
    /** For VBO_MEMORY, this is the offset.  For MAIN_MEMORY, this is
        a pointer to the block of uploaded memory.
        
        When there was a dstOffset as an init() argument, it has already
        been applied here.
    */
    void*               m_pointer;
    
    /** Size of one element in bytes. For a void array, this is 1.*/
    size_t              m_elementSize;
    
    /** For a void array, this is m_maxSize. */
    int                 m_numElements;
   
    /** Space between subsequent elements in bytes, must be either zero or >= m_elementSize */
    size_t              m_stride;
 
    uint64              m_generation;
    
    /** GL_NONE for a "void" array */
    GLenum              m_underlyingRepresentation;
    
    /** The initial size this VertexRange was allocated with, in bytes. */
    size_t              m_maxSize;

    bool                m_normalizedFixedPoint;

    /** For uploading interleaved arrays */
    void init
    (VertexRange&       dstPtr, 
     size_t             dstOffset,
     GLenum             glformat, 
     size_t             eltSize, 
     int                numElements, 
     size_t             stride,
     bool               normalizedFixedPoint);

    void init
    (const void*        sourcePtr, 
     int                numElements, 
     VertexBufferRef    area,
     GLenum             glformat, 
     size_t             eltSize,
     bool               normalizedFixedPoint);

    void init
    (const void*        srcPtr,
     int                numElements, 
     size_t             srcStride,      
     GLenum             glformat, 
     size_t             eltSize,
     VertexRange        dstPtr,
     size_t             dstOffset, 
     size_t             dstStride,
     bool               normalizedFixedPoint);
    
    void update(const void* sourcePtr, int _numElements, GLenum glformat, size_t eltSize, bool normalizedFixedPoint);

    /** Performs the actual memory transfer (like memcpy).  The
        dstPtrOffset is the number of <B>bytes</B> to add to m_pointer
        when performing the transfer. */
    void uploadToCard(const void* sourcePtr, size_t dstPtrOffsetElements, size_t size);

    /** Used for creating interleaved arrays. */
    void uploadToCardStride(const void* sourcePtr, size_t srcElements, size_t srcSizeBytes,
                            size_t srcStrideBytes, size_t dstPtrOffsetBytes, size_t dstStrideBytes);

    void set(int index, const void* value, GLenum glformat, size_t eltSize);

    // The following are called by RenderDevice
    /** May be an OpenGL video memory offset or a real memory pointer.  For use by RenderDevice only.*/
    const void* pointer() const {
        return m_pointer;
    }

    void vertexPointer() const;
    
    void normalPointer() const;
    
    void colorPointer() const;
    
    void texCoordPointer(unsigned int unit) const;
    
    void vertexAttribPointer(unsigned int attribNum) const;

public:

    bool operator==(const VertexRange& other) const {
        return 
            (m_area == other.m_area) &&
            (m_pointer == other.m_pointer) &&
            (m_elementSize == other.m_elementSize) &&
            (m_numElements == other.m_numElements) &&
            (m_stride == other.m_stride) &&
            (m_generation == other.m_generation) &&
            (m_underlyingRepresentation == other.m_underlyingRepresentation) &&
            (m_maxSize == other.m_maxSize) &&
            (m_normalizedFixedPoint == other.m_normalizedFixedPoint);
    }

    /** \sa buffer() \deprecated */
    inline VertexBuffer::Ref area() {
        return m_area;
    }

    /** The G3D::VertexBuffer containing this VertexRange. */
    inline VertexBuffer::Ref buffer() {
        return m_area;
    }

    /** \sa buffer() \deprecated */
    inline VertexBuffer::Ref G3D_DEPRECATED area() const {
        return m_area;
    }

    /** The G3D::VertexBuffer containing this VertexRange. */
    VertexBuffer::Ref buffer() const {
        return m_area;
    }

    VertexBuffer::Type type() const {
        return m_area->type();
    }

    /** \brief Number of elements in this array (not byte size!) */
    int size() const {
        return m_numElements;
    }

    /** \copydoc m_elementSize */
    size_t elementSize() const {
        return m_elementSize;
    }

    /** \copydoc m_stride */
    size_t stride() const {
        return m_stride;
    }

    uint64 generation() const {
        return m_generation;
    }

    GLenum underlyingRepresentation() const {
        return m_underlyingRepresentation;
    }

    /** For VBO_MEMORY, this is the offset.  For MAIN_MEMORY, this is
        a pointer to the block of uploaded memory.
        
        When there was a dstOffset as a constructor argument, it has already
        been applied here.
    */
    void* startAddress() {
        return m_pointer;
    }

    /** Creates an invalid VertexRange. */
    VertexRange();

    /** @brief Creates a VertexRange that acts as a pointer to a block of memory.

        This block of memory can then be used with VertexRange::VertexRange()
        to upload interleaved data.        
     */
    VertexRange(size_t numBytes, VertexBufferRef _area);
    
    /**
       Uploads memory from the CPU to the GPU.  The element type is
       inferred from the pointer type by the preprocessor.  Sample
       usage:       
       
       \code
       // Once at the beginning of the program
       VertexBufferRef dataArea  = VertexBuffer::create(5 * 1024 * 1024);
       VertexBufferRef indexArea = VertexBuffer::create(1024 * 1024, VertexBuffer::WRITE_EVERY_FRAME, VertexBuffer::INDEX);
       
       //----------
       // Store data in main memory
       Array<Vector3> vertexArrayCPU;
       Array<int>     indexArrayCPU;
       
       //... fill out vertex & index arrays
       
       //------------
       // Upload to graphics card whenever CPU data changes
       area.reset();
       VertexRange vertexVARGPU(vertexArrayCPU, dataArea);
       VertexRange indexVARGPU(indexArrayCPU, indexArea);

       //------------
       // Render 
       renderDevice->beginIndexedPrimitives();
       {
           renderDevice->setVertexArray(varray);
           renderDevice->sendIndices(PrimitiveType::TRIANGLES, indexVARGPU);
       }
       renderDevice->endIndexedPrimitives();
       \endcode

       \param sourcePtr The type of the data is determined from this.  If NULL, no data is actually uploaded...but you
       must provide a properly typed pointer, e.g., (uint16*)(NULL).
    */
    template<class T>
    VertexRange(const T* sourcePtr, int _numElements, VertexBufferRef _area) {
        alwaysAssertM((_area->type() == VertexBuffer::DATA) || glCanBeIndexType(T),
                      "Cannot create an index VertexRange in a non-index VertexBuffer");
        init(sourcePtr, _numElements, _area, glFormatOf(T), sizeof(T), glIsNormalizedFixedPoint(T));
    }        

  
    template<class T>
    VertexRange(const Array<T>& source, VertexBufferRef _area) {

        alwaysAssertM((_area->type() == VertexBuffer::DATA) || glCanBeIndexType(T),
                      "Cannot create an index VertexRange in a non-index VertexBuffer");
        init(source.getCArray(), source.size(), _area, glFormatOf(T), sizeof(T), glIsNormalizedFixedPoint(T));
    }

    void setNormalizedFixedPoint(bool b) {
        m_normalizedFixedPoint = b;
    }

    /** True if this vertex is storing numbers in normalized fixed point format */
    bool normalizedFixedPoint() {
        return m_normalizedFixedPoint;
    }


    /** Return a pointer to CPU-addressable memory for this
        VertexRange.  The buffer must be unmapped later before any
        rendering calls are made.  This contains a glPushClientAttrib
        call that must be matched by unmapBuffer.

        Works for both CPU memory and VBO memory VertexRange.

        This method of moving data is not typesafe and is not recommended.
        
        \param permissions Same as the argument to 
        <a href="http://www.opengl.org/sdk/docs/man/xhtml/glMapBuffer.xml">glMapBufferARB</a>:
        <code>GL_READ_ONLY</code>, <code>GL_WRITE_ONLY</code>, or <code>GL_READ_WRITE</code>.                
        */
    void* mapBuffer(GLenum permissions);

    /** Release CPU addressable memory previously returned by mapBuffer.
        This method of moving data is not typesafe and is not recommended. 
      */
    void unmapBuffer();

    /** @brief Update a set of interleaved arrays.  
    
        Update a set of interleaved arrays.  None may change size from the original. */
    template<class T1, class T2, class T3, class T4, class T5>
    static void updateInterleaved
        (const Array<T1>&         src1,
         VertexRange&             var1,
         const Array<T2>&         src2,
         VertexRange&             var2,
         const Array<T3>&         src3,
         VertexRange&             var3,
         const Array<T4>&         src4,
         VertexRange&             var4,
         const Array<T5>&         src5,
         VertexRange&             var5) {


        size_t N = max(src5.size(),
                    max(max(src1.size(), src2.size()),
                         max(src3.size(), src4.size())));

        // Pack arguments into arrays to avoid repeated code below.
        uint8* src[5]   = {(uint8*)src1.getCArray(), (uint8*)src2.getCArray(),
                           (uint8*)src3.getCArray(), (uint8*)src4.getCArray(),
                           (uint8*)src5.getCArray()};
        size_t            count[5] = {src1.size(), src2.size(), src3.size(), src4.size(), src5.size()};
        size_t            size[5]  = {sizeof(T1), sizeof(T2), sizeof(T3), sizeof(T4), sizeof(T5)};
        VertexRange*   var[5]   = {&var1, &var2, &var3, &var4, &var5};
        (void)var;

        // Zero out the size of unused arrays
        for (int a = 0; a < 5; ++a) {
            if (count[a] == 0) {
                // If an array is unused, it occupies no space in the interleaved array.
                size[a] = 0;
            }

            debugAssertM(count[a] == (size_t)var[a]->m_numElements, 
                "Updated arrays must have the same size they were created with.");
            if (a > 1) {
                debugAssertM((var[a]->m_pointer == (uint8*)var[a - 1]->m_pointer + size[a - 1]) || 
                             ((count[a] == 0) && (var[a]->m_pointer == 0)),
                             "Updated interleaved arrays must be the same set and"
                             " order as original interleaved arrays.");
            }
        }

        uint8* dstPtr = (uint8*)var1.mapBuffer(GL_WRITE_ONLY);

        for (size_t i = 0; i < N; ++i) {
            for (int a = 0; a < 5; ++a) {
                if (count[a] > 0) {
                    System::memcpy(dstPtr, src[a] + size[a] * i, size[a]);
                    dstPtr += size[a];
                }
            }
        }

        var1.unmapBuffer();
    }

    template<class T1, class T2, class T3, class T4>
    static void updateInterleaved
        (const Array<T1>&         src1,
         VertexRange&             var1,
         const Array<T2>&         src2,
         VertexRange&             var2,
         const Array<T3>&         src3,
         VertexRange&             var3,
         const Array<T4>&         src4,
         VertexRange&             var4) {
         
         Array<Vector3> src5;
         VertexRange var5;

         updateInterleaved(src1, var1, src2, var2, src3, var3, src4, var4, src5, var5);
    }

    /** @brief Creates five interleaved VertexRange arrays simultaneously. 

        Creates five interleaved VertexRange arrays simultaneously.  This is
        convenient for uploading vertex, normal, texcoords, and tangent
        arrays although it can be used for any five arrays.  This is
        substantially faster than creating a single "void VertexRange" and
        uploading arrays within it using a stride.

        The var<i>n</i> arguments are outputs only; they should not be
        initialized values.

        All src arrays must have the same length or be empty.  Empty arrays
        will return an uninitialized var.

        @sa updateInterleaved
    */
    template<class T1, class T2, class T3, class T4, class T5>
    static void createInterleaved(
                       const Array<T1>&         src1,
                       VertexRange&             var1,
                       const Array<T2>&         src2,
                       VertexRange&             var2,
                       const Array<T3>&         src3,
                       VertexRange&             var3,
                       const Array<T4>&         src4,
                       VertexRange&             var4,
                       const Array<T5>&         src5,
                       VertexRange&             var5,
                       VertexBufferRef          area) {

        int N = max(src5.size(),
                    max(max(src1.size(), src2.size()),
                         max(src3.size(), src4.size())));

        debugAssert(area->type() == VertexBuffer::DATA);
        debugAssert(src1.size() == N || src1.size() == 0);
        debugAssert(src2.size() == N || src2.size() == 0);
        debugAssert(src3.size() == N || src3.size() == 0);
        debugAssert(src4.size() == N || src4.size() == 0);
        debugAssert(src5.size() == N || src5.size() == 0);

        // Treat sizes as zero if the corresponding array is not used
        size_t size1 = (src1.size() == N) ? sizeof(T1) : 0;
        size_t size2 = (src2.size() == N) ? sizeof(T2) : 0;
        size_t size3 = (src3.size() == N) ? sizeof(T3) : 0;
        size_t size4 = (src4.size() == N) ? sizeof(T4) : 0;
        size_t size5 = (src5.size() == N) ? sizeof(T5) : 0;

        size_t stride = size1 + size2 + size3 + size4 + size5;
        size_t totalMemory = stride * N;
        
        VertexRange masterVAR(totalMemory, area);
        var1.init(masterVAR, 0, glFormatOf(T1), size1, src1.size(), stride, glIsNormalizedFixedPoint(T1));
        var2.init(masterVAR, size1, glFormatOf(T2), size2, src2.size(), stride, glIsNormalizedFixedPoint(T2));
        var3.init(masterVAR, size1 + size2, glFormatOf(T3), size3, src3.size(), stride, glIsNormalizedFixedPoint(T3));
        var4.init(masterVAR, size1 + size2 + size3, glFormatOf(T4), size4, src4.size(), stride, glIsNormalizedFixedPoint(T4));
        var5.init(masterVAR, size1 + size2 + size3 + size4, glFormatOf(T5), size5, src5.size(), stride, glIsNormalizedFixedPoint(T5));

        updateInterleaved(src1, var1, src2, var2, src3, var3, src4, var4, src5, var5);
    }

    template<class T1, class T2, class T3, class T4>
    static void createInterleaved(
                       const Array<T1>&         src1,
                       VertexRange&             var1,
                       const Array<T2>&         src2,
                       VertexRange&             var2,
                       const Array<T3>&         src3,
                       VertexRange&             var3,
                       const Array<T4>&         src4,
                       VertexRange&             var4,
                       VertexBufferRef          area) {
        Array<Vector3> src5;
        VertexRange var5;
        createInterleaved(src1, var1, src2, var2, src3, var3, src4, var4, src5, var5, area);
    }

    /**
       @brief Create an interleaved array within an existing VertexRange
       and upload data to it.
              
       Upload @a _numElements values from @a sourcePtr on the 
       CPU to @a dstPtr on the GPU.

       @param srcStride If non-zero, this is the spacing between
       sequential elements <i>in bytes</i>. It may be negative. 

       @param dstOffset Offset in bytes from the head of dstPtr.
       e.g., to upload starting after the 2nd byte, set
       <code>dstOffset = 2</code>.

       @param dstStride If non-zero, this is the spacing between
       sequential elements of T in dstPtr.  e.g., to upload every
       other Vector3, use <code>dstStride = sizeof(Vector3) *
       2</code>.  May not be negative.
     */
    template<class T>
    VertexRange
        (const T*       srcPtr, 
        size_t             _numElements,
        size_t             srcStride,
        VertexRange     dstPtr,
        size_t             dstOffset, 
        size_t             dstStride) {
        init(srcPtr, _numElements, srcStride, glFormatOf(T), sizeof(T), dstPtr, dstOffset, dstStride);
    }


    /** \brief Create an interleaved array within an existing VertexRange, but do
        not upload data to it.

        Data can later be uploaded by update() or mapBuffer().

        Example:
        <PRE>
           G3D_BEGIN_PACKED_CLASS
           struct Packed {
               Vector3   vertex;
               Vector2   texcoord;
           }
           G3D_END_PACKED_CLASS

           ...
            
           int stride = sizeof(Vector3) + sizeof(Vector2);
           int totalSize = stride * N;

           VertexRange interleavedBlock(totalSize, area);

           VertexRange vertex(Vector3() N, interleavedBlock, 0, stride);
           VertexRange texcoord(Vector2(), N, interleavedBlock, sizeof(Vector3), stride);

           Packed* ptr = (Packed*)interleavedBlock.mapBuffer(GL_WRITE_ONLY);
            // ... write to elements of ptr ...
           interleavedBlock.unmapBuffer();
        </PRE>

       @param dstStride If non-zero, this is the spacing between
       sequential elements of T in dstPtr.  e.g., to upload every
       other Vector3, use <code>dstStride = sizeof(Vector3) *
       2</code>.  May not be negative.
       */
    template<class T>
    VertexRange
    (const T&           ignored,
     int                _numElements,
     VertexRange        dstPtr,
     size_t             dstOffset, 
     size_t             dstStride) {
        (void)ignored;
        init(dstPtr, dstOffset, glFormatOf(T), sizeof(T), _numElements, dstStride, glIsNormalizedFixedPoint(T));
    }

    /** Allocate a vertex range within a vertex buffer, but do not upload data to it.
        \param dstStride If non-zero, this is the spacing between
       sequential elements of T in dstPtr.  e.g., to upload every
       other Vector3, use <code>dstStride = sizeof(Vector3) *
       2</code>.  May not be negative.
     \sa mapBuffer*/
    template<class T>
    VertexRange
    (const T&           ignored,
     int                _numElements,
     VertexBuffer::Ref  dstPtr,
     size_t                dstOffset, 
     size_t                dstStride) {
        (void)ignored;
        init(dstPtr, dstOffset, glFormatOf(T), sizeof(T), _numElements, dstStride);
    }

    template<class T>
    VertexRange
    (const Array<T>& source,
     VertexRange     dstPtr,
     size_t          dstOffset, 
     size_t          dstStride) {
        init(source.getCArray(), source.size(), 0, glFormatOf(T), sizeof(T), dstPtr, dstOffset, dstStride, glIsNormalizedFixedPoint(T));
    }

    
    template<class T>
    void update(const T* sourcePtr, int _numElements) {
        debugAssertM((m_area->type() == VertexBuffer::DATA) || glCanBeIndexType(T),
                      "Cannot create an index VertexRange in a non-index VertexBuffer");
        update(sourcePtr, _numElements, glFormatOf(T), sizeof(T), glIsNormalizedFixedPoint(T));
    }
   
    /**
       Overwrites existing data with data of the same size or smaller.
       Convenient for changing part of a G3D::VertexBuffer without reseting
       the area (and thereby deallocating the other G3D::VertexRange arrays in
       it).
    */
    template<class T>
    void update(const Array<T>& source) {
        debugAssertM((m_area->type() == VertexBuffer::DATA) || glCanBeIndexType(T),
                      "Cannot create an index VertexRange in a non-index VertexBuffer");
        update(source.getCArray(), source.size(), glFormatOf(T), sizeof(T), glIsNormalizedFixedPoint(T));
    }
    
    /** Overwrites a single element of an existing array without
       changing the number of elements.  This is faster than calling
       update for large arrays, but slow if many set calls are made.
       Typically used to change a few key vertices, e.g., the single
       dark cap point of a directional light's shadow volume.
     */
    // This is intentionally not the overloaded [] operator because
    // direct access to VertexRange memory is generally slow and discouraged.
    template<class T>
    void set(int index, const T& value) {
        debugAssertM((m_area->type() == VertexBuffer::DATA) || glCanBeIndexType(T),
                      "Cannot create an index VertexRange in a non-index VertexBuffer");
        debugAssertM(glIsNormalizedFixedPoint(T) == m_normalizedFixedPoint, 
                     "Mismatch in normalized fixed point flag between allocation and set time.");

        set(index, &value, glFormatOf(T), sizeof(T));
    }
    
    /** @brief Returns true if this VertexRange can be used for rendering
        (i.e., contains data and the parent VertexBuffer has not been reset).  */
    bool valid() const;

    /** @brief Maximum size that can be loaded via update into this VertexRange. */
    inline size_t maxSize() const {
        if (valid()) {
            return m_maxSize;
        } else {
            return 0;
        }
    }
};


/** @deprecated Use VertexRange */
typedef VertexRange VAR;

} // namespace G3D

#endif
