/**
 \file   DepthOfField.cpp
 \author Morgan McGuire
*/

#include "GLG3D/DepthOfField.h"
#include "GLG3D/RenderDevice.h"
#include "G3D/GCamera.h"

namespace G3D {
    
DepthOfField::DepthOfField() {
    // Intentionally empty
}


DepthOfField::Ref DepthOfField::create() {
    return new DepthOfField();
}


void DepthOfField::reloadShaders() {
    m_artistCoCShader = Shader::fromFiles("", System::findDataFile("DepthOfField/DepthOfField_artistCoC.pix"));
    m_artistCoCShader->setPreserveState(false);

    m_physicalCoCShader = Shader::fromFiles("", System::findDataFile("DepthOfField/DepthOfField_physicalCoC.pix"));
    m_physicalCoCShader->setPreserveState(false);

    m_horizontalShader = Shader::fromFiles("", System::findDataFile("DepthOfField/DepthOfField_horizontal.pix"));
    m_horizontalShader->setPreserveState(false);

    m_verticalShader = Shader::fromFiles("", System::findDataFile("DepthOfField/DepthOfField_vertical.pix"));
    m_verticalShader->setPreserveState(false);

    m_compositeShader = Shader::fromFiles("", System::findDataFile("DepthOfField/DepthOfField_composite.pix"));
    m_compositeShader->setPreserveState(false);
}


void DepthOfField::apply
(RenderDevice*      rd, 
 Texture::Ref       color,
 Texture::Ref       depth, 
 const GCamera&     camera) {
    
    alwaysAssertM(color.notNull(), "Color buffer may not be NULL");
    alwaysAssertM(depth.notNull(), "Depth buffer may not be NULL");

    if (m_artistCoCShader.isNull()) {
        reloadShaders();
    }
    resizeBuffers(color);

    computeCoC(rd, color, depth, camera);
    blurPass(rd, m_packedBuffer, m_horizontalFramebuffer, m_horizontalShader, camera);
    blurPass(rd, m_tempBlurBuffer, m_verticalFramebuffer, m_verticalShader, camera);
    composite(rd, m_packedBuffer, m_blurBuffer);
}


/** Limit the maximum radius allowed for physical blur to 2% of the viewport */
static float maxPhysicalBlurRadius(const GCamera& camera, const Rect2D& viewport) {
    return min(max(camera.circleOfConfusionRadius(camera.nearPlaneZ(), viewport),
                   camera.circleOfConfusionRadius(camera.farPlaneZ(), viewport)), 
                   viewport.width() / 50.0f);
}


void DepthOfField::computeCoC
(RenderDevice*          rd, 
 const Texture::Ref&    color, 
 const Texture::Ref&    depth, 
 const GCamera&         camera) {

    rd->push2D(m_packedFramebuffer); {
        rd->clear();
        const double z_f    = camera.farPlaneZ();
        const double z_n    = camera.nearPlaneZ();
        
        const Vector3& clipInfo = 
            (z_f == -inf()) ? 
            Vector3(float(z_n), -1.0f, 1.0f) : 
            Vector3(float(z_n * z_f),  float(z_n - z_f),  float(z_f));
        
        Shader::Ref shader;
        if (camera.depthOfFieldModel() == GCamera::ARTIST) {
            shader = m_artistCoCShader;
        } else {
            shader = m_physicalCoCShader;
        }

        Shader::ArgList& args = shader->args;

        args.set("color",     color);
        args.set("depth",     depth);
        args.set("clipInfo",  clipInfo);
        
        if (camera.depthOfFieldModel() == GCamera::ARTIST) {

            args.set("nearBlurryPlaneZ", camera.nearBlurryPlaneZ());
            args.set("nearSharpPlaneZ",  camera.nearSharpPlaneZ());
            args.set("farSharpPlaneZ",   camera.farSharpPlaneZ());
            args.set("farBlurryPlaneZ",  camera.farBlurryPlaneZ());

            const float maxRadiusFraction = 
                max(max(camera.nearBlurRadiusFraction(), camera.farBlurRadiusFraction()), 0.001f);

            // This is a positive number
            const float nearNormalize =             
                (1.0f / (camera.nearBlurryPlaneZ() - camera.nearSharpPlaneZ())) *
                (camera.nearBlurRadiusFraction() / maxRadiusFraction);
            alwaysAssertM(nearNormalize >= 0.0f, "Near normalization must be a non-negative factor");
            args.set("nearNormalize", nearNormalize); 

            // This is a positive number
            const float farNormalize =             
                (1.0f / (camera.farSharpPlaneZ() - camera.farBlurryPlaneZ())) *
                (camera.farBlurRadiusFraction() / maxRadiusFraction);
            alwaysAssertM(farNormalize >= 0.0f, "Far normalization must be a non-negative factor");
            args.set("farNormalize", farNormalize);

        } else {
            const float scale = 
                camera.imagePlanePixelsPerMeter(rd->viewport()) * camera.lensRadius() / 
                (camera.focusPlaneZ() * maxPhysicalBlurRadius(camera, color->rect2DBounds()));
            args.set("focusPlaneZ", camera.focusPlaneZ());
            args.set("scale", scale);

        }

        rd->applyRect(shader);

    } rd->pop2D();
}


void DepthOfField::blurPass
(RenderDevice*           rd, 
 const Texture::Ref&     input,
 const Framebuffer::Ref& output,
 Shader::Ref             shader,
 const GCamera&          camera) {

    alwaysAssertM(input.notNull(), "input is NULL");

    // Dimension along which the blur fraction is measured
    const float dimension = 
        (camera.fieldOfViewDirection() == GCamera::HORIZONTAL) ?
        m_packedBuffer->width() : m_packedBuffer->height();

    const float maxRadiusFraction = 
        max(camera.nearBlurRadiusFraction(), camera.farBlurRadiusFraction());

    const int maxCoCRadiusPixels =
            iCeil((camera.depthOfFieldModel() == GCamera::ARTIST) ? 
                  (maxRadiusFraction * dimension) :
                  maxPhysicalBlurRadius(camera, m_packedBuffer->rect2DBounds()));
                  
    rd->push2D(output); {
        rd->clear();
        Shader::ArgList& args = shader->args;

        args.set("blurSourceBuffer",   input);
        args.set("maxCoCRadiusPixels", maxCoCRadiusPixels);

        rd->applyRect(shader);
    } rd->pop2D();
}


void DepthOfField::composite
(RenderDevice*   rd, 
 Texture::Ref    packedBuffer, 
 Texture::Ref    blurBuffer) {
    rd->push2D(); {
        rd->clear(true, false, false);
        Shader::ArgList& args = m_compositeShader->args;
        
        args.set("blurBuffer",   blurBuffer);
        args.set("packedBuffer", packedBuffer);
        
        rd->applyRect(m_compositeShader);
    } rd->pop2D();
}


/** Allocates or resizes a texture and framebuffer to match a target
    format and dimensions. */
static void matchTarget
(const Texture::Ref& target, 
 int                 divWidth, 
 int                 divHeight,
 const ImageFormat*  format,
 Texture::Ref&       texture, 
 Framebuffer::Ref&   framebuffer,
 Framebuffer::AttachmentPoint attachmentPoint = Framebuffer::COLOR0,
 Texture::Settings   settings = Texture::Settings::buffer()) {
    alwaysAssertM(format, "Format may not be NULL");

    if (texture.isNull() || (texture->format() != format)) {
        // Allocate
        texture = Texture::createEmpty
            ("", 
             target->width() / divWidth, 
             target->height() / divHeight,
             format,
             Texture::DIM_2D_NPOT,
             settings);

        if (framebuffer.isNull()) {
            framebuffer = Framebuffer::create("");
        }
        framebuffer->set(attachmentPoint, texture);

    } else if ((texture->width() != target->width() / divWidth) ||
               (texture->height() != target->height() / divHeight)) {
        // Resize
        texture->resize(target->width(), target->height());
    }
}


void DepthOfField::resizeBuffers(Texture::Ref target) {
    const ImageFormat* plusAlphaFormat = ImageFormat::getFormatWithAlpha(target->format());

    // Need an alpha channel for storing radius in the packed and blurry temp buffers
    matchTarget(target, 1, 1, plusAlphaFormat,     m_packedBuffer,    m_packedFramebuffer);
    matchTarget(target, 2, 1, plusAlphaFormat,     m_tempBlurBuffer,  m_horizontalFramebuffer);
    matchTarget(target, 2, 2, target->format(),    m_blurBuffer,      m_verticalFramebuffer, Framebuffer::COLOR0, Texture::Settings::video());
}

} // Namespace G3D
