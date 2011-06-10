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
#include "GLG3D/Framebuffer.h"
#include "GLG3D/Texture.h"
#include "GLG3D/Shader.h"
#include "GLG3D/Material.h"

namespace G3D {

typedef ReferenceCountedPointer<class Surface> SurfaceRef;
typedef ReferenceCountedPointer<class SuperSurface> SuperSurfaceRef;
class RenderDevice;

/** \brief Saito and Takahashi's Geometry Buffers, typically used today for deferred shading. 
    Contains position, normal, depth, and BSDF parameters.

    Used for rendering a G3D::SuperBSDF with deferred shading. 

    Requires SS_GBuffer.pix, SS_GBufferPosition.pix, and
    SS_NonShadowedPass.vrt at runtime, which can be found in the
    G3D/data-files/SuperShader directory of the G3D distribution.

    \beta
*/
class GBuffer : public ReferenceCountedObject {
public:


    /** If a format is NULL, it is not allocated or rendered. CS =
        camera space, WS = world space, SS = screen space.*/
    class Specification {
    public:
#if 0

        /** Shading normal, after interpolation and bump-mapping.
            Floating point formats (e.g., RGBF) will produced signed
            normals, normalized formats (e.g., RGB8) will encode as n'
            = (n+1)/2*/
        const ImageFormat*    wsNormal;

        const ImageFormat*    csNormal;

        const ImageFormat*    wsFaceNormal;

        const ImageFormat*    csFaceNormal;

        /** Requires a normalized or float format because values are
            on (0,1). Only RGB fields are written.*/
        const ImageFormat*    lambertian;

        /** RGB = magnitude; A = exponent.  See Material.*/
        const ImageFormat*    glossy;

        const ImageFormat*    transmissive;
        
        const ImageFormat*    emissive;

        /** Requires a floating-point format */
        const ImageFormat*    wsPosition;

        const ImageFormat*    csPosition;

        /** Floating point RG texture storing the screen-space pixel
            displacement since the previous frame. */
        const ImageFormat*    ssVelocity;

        /** RGB floating point */
        const ImageFormat*    wsVelocity;

        /** RGB floating point */
        const ImageFormat*    csVelocity;

        /** Camera-space Z.  Must be a floating-point, R-only texture. */
        const ImageFormat*    csZ;

        /** For the depth buffer */
        const ImageFormat*    depth;

        enum DepthEncoding {
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
             projection matrix. */
            COMPLEMENTARY
        };

        DepthEncoding         depthEncoding;

#endif

        /** World-space shading normal in RGB (after bump mapping). */
        bool                  wsNormal;

        /** Camera-space shading normal in RGB (after bump mapping). */
        bool                  csNormal;

        bool                  lambertian;
        bool                  specular;
        bool                  transmissive;
        bool                  emissive;

        /** World-space triangle normal in RGB. */
        bool                  wsFaceNormal;

        /** Camera-space triangle normal in RGB. */
        bool                  csFaceNormal;

        /** If false, normal channels are encoded as \f$ \vec{n}'_i = (\vec{n}_i + 1)/n\f$. 
            This is typically desirable for 8-bit formats.
            Defaults to false.*/
        bool                  normalsAreSigned;

        /** Packed camera-space depth. */
        bool                  packedDepth;

        /** The G3D::Material "custom" channel */
        bool                  custom;

        /** World-space position in RGB. */
        bool                  wsPosition;

        /** Camera-space position in RGB. */
        bool                  csPosition;

        /** Screen-space velocity in RG */
        bool                  ssVelocity;

        /** Camera-space velocity in RGB */
        bool                  csVelocity;

        /** World-space velocity in RGB */
        bool                  wsVelocity;

        /** Must contain four channels */
        const ImageFormat*    format;

        const ImageFormat*    depthFormat;

        /** Must have at least three channels */
        const ImageFormat*    positionFormat;

        /** All fields for specific buffers default to false.  In the future, more buffers may be added, which will also 
            default to false for backwards compatibility. */
        Specification() : 
            wsNormal(false),
            csNormal(false),
            lambertian(false),
            specular(false),
            transmissive(false),
            emissive(false),
            wsFaceNormal(false),
            csFaceNormal(false),
            normalsAreSigned(false),
            packedDepth(false),
            custom(false),
            wsPosition(false),
            csPosition(false),
            ssVelocity(false),
            csVelocity(false),
            wsVelocity(false),
            format(ImageFormat::RGBA8()),
            depthFormat(ImageFormat::DEPTH24()), 
            positionFormat(ImageFormat::RGB16F()) {
        }

        size_t hashCode() const {
            return 
                 int(wsNormal)           |
                (int(csNormal)    << 1)  |
                (int(lambertian)  << 2)  |
                (int(specular)    << 3)  |
                (int(transmissive)<< 4)  |
                (int(emissive)    << 5)  |
                (int(csFaceNormal)<< 6)  |
                (int(wsFaceNormal)<< 7)  |
                (int(packedDepth) << 8)  |
                (int(custom)      << 9)  |
                (int(csPosition)  << 10) |
                (int(wsPosition)  << 11) |
                (int(ssVelocity)  << 12) |
                (int(csVelocity)  << 13) |
                (int(wsVelocity)  << 14) |
                (int(normalsAreSigned) << 15);
        }

        /** Can be used with G3D::Table as an Equals function */
        class Similar {
        public:
            static bool equals(const Specification& a, const Specification& b) {
                return hashCode(a) == hashCode(b);
            }
            static size_t hashCode(const Specification& s) {
                return s.hashCode();
            }
        };
    };

    typedef ReferenceCountedPointer<GBuffer> Ref;

private:

    class Indices {
    public:
        int L;
        int s;
        int t;
        int e;
        int csN;
        int wsN;
        int csF;
        int wsF;
        int z;
        int c;
        
        int csP;
        int wsP;

        /** For the primary pass, which includes normals */
        int numPrimaryAttach;

        /** For the position pass */
        int numPositionAttach;
        
        /** Indices of the FBO fields.  */
        Indices(const Specification& spec);

        /** For the primary pass */
        std::string computeDefines() const;

        /** For the position pass */
        std::string computePositionDefines() const;
    };

    std::string                 m_name;

    const Specification         m_specification;

    const Indices               m_indices;

    Shader::Ref                 m_positionShader;
    
    mutable GCamera             m_camera;

    /** The other buffers are permanently bound to this framebuffer */
    Framebuffer::Ref            m_framebuffer;

    Framebuffer::Ref            m_positionFramebuffer;

    /** RGB = diffuse reflectance (Fresnel is not applied), A = alpha */
    Texture::Ref                m_lambertian;

    /** RGB = F0, A = \f$\sigma\f$ (packed specular exponent).  Fresnel
        is not applied */
    Texture::Ref                m_specular;

    /** RGB = T0, A = eta.  Fresnel is not applied */
    Texture::Ref                m_transmissive;

    Texture::Ref                m_emissive;

    Texture::Ref                m_csNormal;
    Texture::Ref                m_wsNormal;

    Texture::Ref                m_wsFaceNormal;
    Texture::Ref                m_csFaceNormal;

    Texture::Ref                m_packedDepth;

    /** World-space position */
    Texture::Ref                m_wsPosition;
    Texture::Ref                m_csPosition;

    /** Depth texture. */
    Texture::Ref                m_depth;

    /** Returns the appropriate shader for this combination of
        specification and material, checking against a cache of
        previously compiled shaders.  The shader is not yet configured
        for the material.*/
    static Shader::Ref getShader(const Specification& specification, const Indices& indices, const Material::Ref& material);

    GBuffer(const std::string& name, const Specification& specification);

    /** Called from the constructor */
    Shader::Ref makePositionShader();

    void compute
    (RenderDevice* rd, 
     const SuperSurfaceRef& model) const;

    void computeArray
    (RenderDevice* rd, 
     const Array<SuperSurfaceRef>& model) const;

    // Intentionally unimplemented
    GBuffer& operator=(const GBuffer&);

public:

    /** \brief Returns true if GBuffer is supported on this GPU */
    static bool supported();

    static Ref create
    (const std::string& name = "GBuffer",
     const Specification& specification = Specification());

    virtual ~GBuffer();

    int width() const;

    int height() const;

    Rect2D rect2DBounds() const;

    /** The other buffers are permanently bound to this framebuffer */
    Framebuffer::Ref framebuffer() const {
        return m_framebuffer;
    }

    Framebuffer::Ref positionFramebuffer() const {
        return m_positionFramebuffer;
    }

    /** The camera from which these buffers were rendered */
    const GCamera& camera() const {
        return m_camera;
    }

    /** RGB = diffuse reflectance (Fresnel is not applied), A =
        partial coverage. */
    Texture::Ref lambertian() const {
        return m_lambertian;
    }

    /** RGB = F0, A = \f$\sigma\f$ (packed specular exponent).  Fresnel
        is not applied */
    Texture::Ref specular() const {
        return m_specular;
    }

    /** RGB = T0, A = eta.  Fresnel is not applied */
    Texture::Ref transmissive() const {
        return m_transmissive;
    }

    /** RGB = T0, A = eta.  */
    Texture::Ref emissive() const {
        return m_emissive;
    }

    /** World-space position */
    Texture::Ref wsPosition() const {
        return m_wsPosition;
    }

    /** Camera-space position */
    Texture::Ref csPosition() const {
        return m_csPosition;
    }

    /** Camera-space unit shading normal, after bump mapping. */
    Texture::Ref csNormal() const {
        return m_csNormal;
    }

    /** World-space unit shading normal, after bump mapping. */
    Texture::Ref wsNormal() const {
        return m_wsNormal;
    }

    /** World-space geometric normal */
    Texture::Ref wsFaceNormal() const {
        return m_wsFaceNormal;
    }

    /** World-space geometric normal */
    Texture::Ref csFaceNormal() const {
        return m_csFaceNormal;
    }

    /** Camera space depth */
    Texture::Ref packedDepth() const {
        return m_packedDepth;
    }

    /** Depth texture. */
    Texture::Ref depth() const {
        return m_depth;
    }

    const std::string& name() const {
        return m_name;
    }

    /** Reallocate all buffers to this size if they are not already */
    virtual void resize(int width, int height);

    /** Render the models to this GBuffer set (clearing first).  Depth
        is only cleared if RenderDevice::depthWrite is true.

        Assumes that \a modelArray has already been culled and sorted
        for the camera.

        Performs binary alpha testing using Lambertian.a.

        Only renders elements of \a modelArray that are SuperSurface
        instances.
    */
    void compute
    (RenderDevice*                  rd, 
     const GCamera&                 camera,
     const Array<SurfaceRef>&       modelArray) const;

    const Specification& specification() const {
        return m_specification;
    }

	/** Explicitly override the camera stored in the GBuffer. */
    void setCamera(const GCamera& camera) {
        m_camera = camera;
    }
};

} // namespace G3D

#endif
