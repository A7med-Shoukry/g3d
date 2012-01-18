/**
  \file GLG3D/GBuffer.h
  \author Morgan McGuire, http://graphics.cs.williams.edu
 */
#ifndef GLG3D_GBuffer_h
#define GLG3D_GBuffer_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "G3D/ImageFormat.h"
#include "G3D/GCamera.h"
#include "G3D/enumclass.h"
#include "GLG3D/Framebuffer.h"
#include "GLG3D/Texture.h"
#include "GLG3D/Shader.h"
#include "GLG3D/Material.h"

namespace G3D {

typedef ReferenceCountedPointer<class Surface> SurfaceRef;
typedef ReferenceCountedPointer<class SuperSurface> SuperSurfaceRef;
class RenderDevice;

/** Encoding of the depth buffer (not the GBuffer::Field::CS_Z buffer) */
class DepthEncoding {
public:

    enum Value {
        /** Traditional (n)/(f-n) * (1 - f/z) encoding.  Easy to
            produce from a projection matrix, few good numerical
            properties.*/
        HYPERBOLIC,

        /** (z-n)/(f-n), provides uniform precision for fixed
            point formats like DEPTH24, and easy to reconstruct
            csZ directly from the depth buffer. Poor precision
            under floating-point formats.

            Accomplished using a custom vertex shader; not possible
            with a projection matrix.
        */
        LINEAR,
            
        /** (n)/(f-n) * (f/z-1) encoding, good for floating-point
            formats like DEPTH32F.
             
            Accomplished using glDepthRange(1.0f, 0.0f) with a traditionl
            projection matrix. 

            \cite http://portal.acm.org/citation.cfm?id=311579&dl=ACM&coll=DL&CFID=28183902&CFTOKEN=63987370*/
        COMPLEMENTARY
    } value;

    static const char* toString(int i, Value& v);

    G3D_DECLARE_ENUM_CLASS_METHODS(DepthEncoding);

}; // class DepthEncoding

} // namespace G3D
G3D_DECLARE_ENUM_CLASS_HASHCODE(G3D::DepthEncoding);
namespace G3D {

/** \brief Saito and Takahashi's Geometry Buffers, typically 
    used today for deferred shading. 

    Optionally contains position, normal, depth, velocity, and BSDF parameters
    as well as depth and stencil.
    
    Example:
    \code
    GBuffer::Specification specification;
    specification.format[GBuffer::Field::WS_NORMAL]   = ImageFormat::RGB8();
    specification.encoding[GBuffer::Field::WS_NORMAL].readMultiplyFirst =  2.0f;
    specification.encoding[GBuffer::Field::WS_NORMAL].readAddSecond     = -1.0f;
    specification.format[GBuffer::Field::WS_POSITION] = ImageFormat::RGB16F();
    specification.format[GBuffer::Field::LAMBERTIAN]  = ImageFormat::RGB8();
    specification.format[GBuffer::Field::GLOSSY]      = ImageFormat::RGBA8();
    specification.format[GBuffer::Field::DEPTH_AND_STENCIL] = ImageFormat::DEPTH32();
    specification.depthEncoding = GBuffer::DepthEncoding::HYPERBOLIC;
    
    gbuffer = GBuffer::create(specification);
    gbuffer->setSize(w, h);

    ...

    gbuffer->prepare(rd, defaultCamera, 0, -1.0f / desiredFrameRate());
    Surface::renderIntoGBuffer(rd, surface3D, gbuffer);
    \endcode

    \sa Surface, Shader, Texture
*/
class GBuffer : public ReferenceCountedObject {
public:
    
    typedef ReferenceCountedPointer<GBuffer> Ref;

    /**
     \brief Names of fields that may be present in a GBuffer.

     These use the abbreviations CS = camera space, WS = world space, SS = screen space.

     Normals are always encoded as n' = (n+1)/2, even if they are in
     floating point format, to simplify the implementation of routines
     that read and write GBuffers.
     */
    class Field {
    public:

        enum Value {

            /** Shading normal, after interpolation and bump-mapping.*/
            WS_NORMAL,

            /** \copydoc WS_NORMAL */
            CS_NORMAL,

            /** Geometric normal of the face, independent of the
                vertex normals. */
            WS_FACE_NORMAL,

            /** \copydoc WS_FACE_NORMAL */
            CS_FACE_NORMAL,

            /** Must be a floating-point format */
            WS_POSITION,

            /** Must be a floating-point format */
            CS_POSITION,

            /** Must be a floating-point or normalized fixed-point
                format. \sa SuperBSDF, Material.*/
            LAMBERTIAN,

            /** RGB = magnitude; A = exponent. Fresnel has not been
                applied. Must be a floating-point or normalized
                fixed-point format. \sa SuperBSDF, Material. */
            GLOSSY,

            /** Must be a RGBA floating-point or normalized
                fixed-point format. Index of refraction is in the A
                channel. \sa SuperBSDF, Material. */
            TRANSMISSIVE,

            /** Must be a floating-point or normalized fixed-point
                format. \sa SuperBSDF, Material. */
            EMISSIVE,

            /** World-space position change since the previous frame,
                according to a Surface.  Must be RGB floating-point.
            
                The name "velocity" is reserved for future use as
                instantaneous velocity. 

                There is no "WS_POSITION_CHANGE" because there is no
                application (for a screen-space buffer of position
                changes that don't take the camera's own movement into
                account) to justify the added implementation
                complexity required for that.
            */
            CS_POSITION_CHANGE,

            /** Texture storing the screen-space pixel displacement
                since the previous frame. As a result, floating-point
                textures will store the sub-pixel displacement and
                signed integers (e.g., ImageFormat::RG8UI) will round
                to the nearest pixel.
            */
            SS_POSITION_CHANGE,

            /** Camera-space Z.  Must be a floating-point, R-only texture. This is always a negative value
            if a perspective transformation has been applied.*/
            CS_Z,

            /** The depth buffer, used for depth write and test.  Not camera-space Z.
            This may include stencil bits */
            DEPTH_AND_STENCIL,
        
            /** Not a valid */
            COUNT
        } value;
  
        static const char* toString(int i, Value& v);

        bool isUnitVector() const {
            return (value == WS_NORMAL) || (value == CS_NORMAL);
        }

        G3D_DECLARE_ENUM_CLASS_METHODS(Field);
    }; // class Field

    
    class Specification {
    public:

        /** Buffers can be optionally scaled and biased to
            facilitate packing signed or large-range values in
            unsigned normalized formats.  For example, surface
            normals can be packed into ImageFormat::RGB8 using \a
            readMultiplyFirst = 2, \a readAddSecond = -1.
            
            <b>Reading:</b>
            <br/><code>texelFetch(buffer, pixel, 0) * readMultiplyFirst + readAddSecond;</code>
            
            <b>Writing</b> (e.g., as done by G3D::Surface::renderIntoGBuffer):
            <br/><code>gl_FragData[n] = (value - readAddSecond) / readMultiplyFirst;</code>
            
            The default add bias is zero and default multiply scale is one.
        */
        class Encoding {
        public:
            float               readMultiplyFirst;

            float               readAddSecond;           
            
            Encoding() : 
                readMultiplyFirst(1.0f), 
                readAddSecond(0.0f) {}
        };

        /** Formats corresponding to the values of Field.
            If a format is NULL, it is not allocated or rendered. */
        const ImageFormat*      format[Field::COUNT];

        /** Parallel array to format[] */
        Encoding                encoding[Field::COUNT];

        /** Reserved for future use--not currently supported */
        DepthEncoding           depthEncoding;
        
        /** All fields for specific buffers default to NULL.  In the
            future, more buffers may be added, which will also default
            to NULL for backwards compatibility. */
        Specification() {
            for (int f = 0; f < Field::COUNT; ++f) {
                format[f] = NULL;
            }
            depthEncoding = DepthEncoding::HYPERBOLIC;
        }

        size_t hashCode() const {
            return
                superFastHash(format, sizeof(ImageFormat*) * Field::COUNT) +
                depthEncoding.hashCode();
        }
        
        /** Can be used with G3D::Table as an Equals and Hash function for a GBuffer::Specification.

          For example, 
        \code
        typedef Table<GBuffer::Specification, Shader::Ref, GBuffer::Specification::SameFields, GBuffer::Specification::SameFields> ShaderCache;
        \endcode

        Does not compare encoding or depthEncoding.
        */
        class SameFields {
        public:
            static size_t hashCode(const Specification& s) {
                size_t h = 0;
                for (int f = 0; f < Field::COUNT; ++f) {
                    h = (h << 1) | (s.format[f] != NULL);
                }
                return h;
            }

            static bool equals(const Specification& a, const Specification& b) {
                return hashCode(a) == hashCode(b);
            }
        };
    };


protected:
    
    std::string                 m_name;

    const Specification         m_specification;
        
    GCamera                     m_camera;

    float                       m_timeOffset;

    float                       m_velocityStartTimeOffset;

    /** The other buffers are permanently bound to this framebuffer */
    Framebuffer::Ref            m_framebuffer;

    Framebuffer::AttachmentPoint m_fieldToAttachmentPoint[Field::COUNT];

    std::string                 m_macroString;

    /** True when the textures have been allocated */
    bool                        m_texturesAllocated;

    bool                        m_depthOnly;

    bool                        m_hasFaceNormals;

    /** The value that GBuffer::prepare clears this buffer to.  Defaults to (0,0,0,0).
        This value does not receive scale and bias.
        Corresponds to the glClearColor(). */
    Color4                      m_clearValue[Field::COUNT];
    
    GBuffer(const std::string& name, const Specification& specification);

    // Intentionally unimplemented
    GBuffer(const GBuffer&);

    // Intentionally unimplemented
    GBuffer& operator=(const GBuffer&);

public:

    /** \brief Returns true if GBuffer is supported on this GPU */
    static bool supported();

    static Ref create
    (const Specification& specification,
     const std::string& name = "GBuffer");

    int width() const;

    int height() const;

    Rect2D rect2DBounds() const;

    /**
    \brief A series of macros to prepend before a Surface's shader
    for rendering to GBuffers.  
    
    This defines each of the fields
    in use and maps it to a GLSL output variable.  For example,
    it might contain:

    \code
    #define WS_NORMAL   gl_FragData[0]
    #define LAMBERTIAN  gl_FragData[1]
    #define DEPTH       gl_FragDepth
    \endcode
    */
    const std::string macros() const {
        return m_macroString;
    }

    /** Returns the attachment point on framebuffer() for \a field.*/
    Framebuffer::AttachmentPoint attachmentPoint(Field field) const {
        return m_fieldToAttachmentPoint[field.value];
    }

    /** \copydoc m_clearValue */
    void setClearValue(Field field, const Color4& c) {
        m_clearValue[field] = c;
    }

    /** \copydoc m_clearValue */
    const Color4& clearValue(Field field) const {
        return m_clearValue[field];
    }

    const Specification& specification() const {
        return m_specification;
    }

    /** The other buffers are permanently bound to this framebuffer */
    const Framebuffer::Ref& framebuffer() const {
        return m_framebuffer;
    }

    /** \copydoc framebuffer() const */
    Framebuffer::Ref framebuffer() {
        return m_framebuffer;
    }

    /** The camera from which these buffers were rendered. */
    const GCamera& camera() const {
        return m_camera;
    }

    /** For debugging in programs with many GBuffers. */
    const std::string& name() const {
        return m_name;
    }

    /** Reallocate all buffers to this size if they are not already. */
    virtual void resize(int width, int height);

	/** Explicitly override the camera stored in the GBuffer. */
    void setCamera(const GCamera& camera) {
        m_camera = camera;
    }

    /** \sa Surface::renderGBufferHomogeneous */
    void setTimeOffsets(float timeOffset, float velocityStartTimeOffset) {
        m_timeOffset = timeOffset;
        m_velocityStartTimeOffset = velocityStartTimeOffset;
    }

    /** \sa Surface::renderGBufferHomogeneous */
    float timeOffset() const {
        return m_timeOffset;
    }

    /** \sa Surface::renderGBufferHomogeneous */
    float velocityStartTimeOffset() const {
        return m_velocityStartTimeOffset;
    }

    /** True iff this G-buffer renders only depth and stencil. */
    bool isDepthAndStencilOnly() const {
        return m_depthOnly;
    }

    /** True if this contains non-NULL Field::CS_FACE_NORMAL or Field::WS_FACE_NORMAL in the specificationl. */
    bool hasFaceNormals() const {
        return m_hasFaceNormals;
    }

    /** Returns the Texture bound to \a f, or NULL if there is not one */
    Texture::Ref texture(Field f) const {
        return m_framebuffer->get(m_fieldToAttachmentPoint[f])->texture();
    }

    /** Returns the Renderbuffer bound to \a f, or NULL if there is not one */
    Renderbuffer::Ref renderbuffer(Field f) const {
        return m_framebuffer->get(m_fieldToAttachmentPoint[f])->renderbuffer();
    }

    /** Bind the framebuffer and clear it, then set the camera and time offsets */
    void prepare(RenderDevice* rd, const GCamera& camera, float timeOffset, float velocityStartTimeOffset);
};

} // namespace G3D

G3D_DECLARE_ENUM_CLASS_HASHCODE(G3D::GBuffer::Field);

#endif
