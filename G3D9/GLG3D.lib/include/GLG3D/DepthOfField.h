/**
 \file   DepthOfField.h
 \maintainer Morgan McGuire
 \edited 2012-07-08
 
 Copyright 2000-2012, Morgan McGuire.
 All rights reserved.
 */
#ifndef GLG3D_DepthOfField_h
#define GLG3D_DepthOfField_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/Texture.h"
#include "GLG3D/Framebuffer.h"
#include "GLG3D/Shader.h"

namespace G3D {

class RenderDevice;
class GCamera;

/** 
  \brief Simple defocus post-process shader
 */
class DepthOfField : public ReferenceCountedObject {
protected:

    DepthOfField();

    /** Color in RGB, circle of confusion and 'near field' bit in A. 
        Precision determined by the input, RGB8, RGB16F, or RGB32F.

        The A channel values are always written with only 8 bits of
        effective precision.

        The radius (A channel) values are scaled and biased to [0, 1].
        Unpack them to pixel radii with:

        \code
        r = ((a * 2) - 1) * maxRadius
        \endcode

        Where maxRadius the larger of the maximum near and far field
        blurs.  The decoded radius is negative in the far field (the packed
        alpha channel should look like a head lamp on a dark night, with
        nearby objects bright, the focus field gray, and the distance black).
    */
    Texture::Ref         m_packedBuffer;
    Framebuffer::Ref     m_packedFramebuffer;
    Shader::Ref          m_artistCoCShader;
    Shader::Ref          m_physicalCoCShader;

    Framebuffer::Ref     m_horizontalFramebuffer;
    Shader::Ref          m_horizontalShader;
    Texture::Ref         m_tempBlurBuffer;

    Framebuffer::Ref     m_verticalFramebuffer;
    Shader::Ref          m_verticalShader;
    Texture::Ref         m_blurBuffer;

    Shader::Ref          m_compositeShader;

    /** Allocates and resizes buffers */
    void resizeBuffers(Texture::Ref target);

    /** Writes m_packedBuffer */
    void computeCoC(RenderDevice* rd, const Texture::Ref& color, const Texture::Ref& depth, const GCamera& camera);

    /** Called first for horizontal and then again for vertical */
    void blurPass(RenderDevice* rd, const Texture::Ref& input, const Framebuffer::Ref& output, Shader::Ref shader, const GCamera& camera);

    /**
       Writes to the currently-bound framebuffer.
     */
    void composite(RenderDevice* rd, Texture::Ref packedBuffer, Texture::Ref blurBuffer);

public:

    typedef ReferenceCountedPointer<DepthOfField> Ref;

    void reloadShaders();


    /** \brief Constructs an empty DepthOfField. */
    static Ref create();

    /** Applies depth of field blur to supplied images and renders to
        the currently-bound framebuffer.  The current framebuffer may
        have the \a color and \a depth values bound to it.

        Reads depth reconstruction and circle of confusion parameters
        from \a camera.
    */
    void apply(RenderDevice* rd, Texture::Ref color, Texture::Ref depth, const GCamera& camera);
};

} // namespace G3D

#endif
