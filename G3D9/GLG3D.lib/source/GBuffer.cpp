/**
  \file GLG3D.lib/source/GBuffer.cpp
  \author Morgan McGuire, http://graphics.cs.williams.edu
 */
#include "GLG3D/GBuffer.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/SuperBSDF.h"
#include "G3D/fileutils.h"
#include "G3D/FileSystem.h"
#include "GLG3D/SuperSurface.h"
#include "GLG3D/Surface.h"

namespace G3D {
    
const char* DepthEncoding::toString(int i, Value& v) {
    static const char* str[] = 
       {"HYPERBOLIC",
        "LINEAR",
        "COMPLEMENTARY",
        NULL};

    static const Value val[] = 
       {HYPERBOLIC,
        LINEAR,
        COMPLEMENTARY};

    const char* s = str[i];
    if (s) {
        v = val[i];
    }
    return s;
}


const char* GBuffer::Field::toString(int i, Value& v) {
    static const char* str[] = 
       {"WS_NORMAL",
        "CS_NORMAL",
        "WS_FACE_NORMAL",
        "CS_FACE_NORMAL",
        "WS_POSITION",
        "CS_POSITION",

        "LAMBERTIAN",
        "GLOSSY",
        "TRANSMISSIVE",
        "EMISSIVE",

        "CS_POSITION_CHANGE",
        "SS_POSITION_CHANGE",

        "CS_Z",

        "DEPTH",
        "TS_NORMAL",
        NULL};

    static const Value val[] = 
       {WS_NORMAL,
        CS_NORMAL,
        WS_FACE_NORMAL,
        CS_FACE_NORMAL,
        WS_POSITION,
        CS_POSITION,

        LAMBERTIAN,
        GLOSSY,
        TRANSMISSIVE,
        EMISSIVE,

        CS_POSITION_CHANGE,
        SS_POSITION_CHANGE,

        CS_Z,

        DEPTH_AND_STENCIL,
        TS_NORMAL};

    const char* s = str[i];
    if (s) {
        v = val[i];
    }
    return s;
}


GBuffer::Ref GBuffer::create
(const Specification& specification,
 const std::string&   name) {
    return new GBuffer(name, specification);
}


GBuffer::Specification::Specification() {
    for (int f = 0; f < Field::COUNT; ++f) {
        format[f] = NULL;
    }
    depthEncoding = DepthEncoding::HYPERBOLIC;
    encoding[Field::WS_FACE_NORMAL] = Encoding(2.0f, -1.0f);
    encoding[Field::CS_FACE_NORMAL] = Encoding(2.0f, -1.0f);
    encoding[Field::CS_NORMAL]      = Encoding(2.0f, -1.0f);
    encoding[Field::WS_NORMAL]      = Encoding(2.0f, -1.0f);
    encoding[Field::TS_NORMAL]      = Encoding(2.0f, -1.0f);
    encoding[Field::SS_POSITION_CHANGE] = Encoding(64.0f, -32.0f);
}


GBuffer::GBuffer(const std::string& name, const Specification& specification) :
    m_name(name),
    m_specification(specification), 
    m_timeOffset(0),
    m_velocityStartTimeOffset(0),
    m_framebuffer(Framebuffer::create(name)),
    m_readDeclarationString("\n"),
    m_writeDeclarationString("\n"),
    m_texturesAllocated(false),
    m_depthOnly(true),
    m_hasFaceNormals(false) {

    // Compute the attachment point for each Field and initialize the clear value.
    int a = Framebuffer::COLOR0;
    for (int f = 0; f < Field::COUNT; ++f) {
        const ImageFormat* format = m_specification.format[f];
        if (format != NULL) {
            if (f == Field::DEPTH_AND_STENCIL) {
                if ((format->stencilBits > 0) && (format->depthBits > 0)) {
                    m_fieldToAttachmentPoint[f] = Framebuffer::DEPTH_AND_STENCIL;
                } else if (format->stencilBits > 0) {
                    m_fieldToAttachmentPoint[f] = Framebuffer::STENCIL;
                } else if (format->depthBits > 0) {
                    m_fieldToAttachmentPoint[f] = Framebuffer::DEPTH;
                }
                
                m_writeDeclarationString += "#define DEPTH gl_FragDepth\n";
                m_readDeclarationString  += "#define DEPTH\n";

            } else {
                m_depthOnly = false;

                if ((f == Field::CS_FACE_NORMAL) || (f == Field::WS_FACE_NORMAL)) {
                    m_hasFaceNormals = true;
                }

                m_fieldToAttachmentPoint[f] = Framebuffer::AttachmentPoint(a);

                m_writeDeclarationString += G3D::format("#define %s gl_FragData[%d]\n", 
                    Field(f).toString(),
                    a - Framebuffer::COLOR0);
                m_writeDeclarationString += std::string("uniform vec2 ") + Field(f).toString() + "_writeScaleBias;\n";

                m_readDeclarationString += "#define Field(f).toString()";
                m_readDeclarationString += std::string("uniform vec2 ") + Field(f).toString() + "_readScaleBias;\n";

                ++a;
            }
        } else {
            m_fieldToAttachmentPoint[f] = Framebuffer::AttachmentPoint(GL_NONE);
        } // if format != NULL

        m_clearValue[f] = Color4::zero();

    } // for each format
}


void GBuffer::bindWriteUniforms(Shader::ArgList& args) const {
    for (int f = 0; f < Field::COUNT; ++f) {
        const ImageFormat* format = m_specification.format[f];
        if ((format != NULL) && (f != Field::DEPTH_AND_STENCIL)) {
            const Encoding& encoding = m_specification.encoding[f];

            // Set the inverse of the read values
            args.set(std::string(Field(f).toString()) + "_writeScaleBias", 
                Vector2(1.0f / encoding.readMultiplyFirst, -encoding.readAddSecond / encoding.readMultiplyFirst), 
                     true);
        }
    }
}


void GBuffer::bindReadUniforms(Shader::ArgList& args) const {
    for (int f = 0; f < Field::COUNT; ++f) {
        const ImageFormat* format = m_specification.format[f];
        if (format != NULL) {
            bindReadArgs(args, Field(f), texture(f), m_specification.encoding[f]);
        }
    }
}


void GBuffer::bindReadArgs(Shader::ArgList& args, const Field& field, const Texture::Ref& texture, const Encoding& encoding) {
    if (texture.isNull()) {
        return;
    }

    const bool optional = true;
    const std::string& base = field.toString();

    args.set(base + "_buffer", texture, optional);
    const ImageFormat* format = texture.isNull() ? NULL : texture->format();
    if ((format != NULL) && (field != Field::DEPTH_AND_STENCIL)) {
        args.set(base + "_readScaleBias", (Vector2)encoding, optional);
    }
}


int GBuffer::width() const {
    return m_framebuffer->width();
}


int GBuffer::height() const {
    return m_framebuffer->height();
}


Rect2D GBuffer::rect2DBounds() const {
    return m_framebuffer->rect2DBounds();
}


void GBuffer::prepare
   (RenderDevice*   rd, 
    const GCamera&  camera, 
    float           timeOffset, 
    float           velocityStartTimeOffset) {

    debugAssertM(m_framebuffer.notNull(), "Must invoke GBuffer::resize before GBuffer::prepare");
    rd->pushState(m_framebuffer); {

/*        for (int f = 0; f < Field::COUNT; ++f) {
            Framebuffer::AttachmentPoint a = m_fieldToAttachmentPoint[f];
        }
        // TODO: per-buffer clear constants
        */
        rd->setColorClearValue(Color4::clear());
        rd->clear();
    } rd->popState();
    setCamera(camera);
    setTimeOffsets(timeOffset, velocityStartTimeOffset);
}


void GBuffer::resize(int w, int h) {
    debugAssert(w >= 0 && h >= 0);
    if (w == width() && h == height()) {
        // Already allocated
        return;
    }

    if (m_texturesAllocated) {
        // Resize
        for (int f = 0; f < Field::COUNT; ++f) {
            if (m_specification.format[f] != NULL) {
                Framebuffer::AttachmentPoint ap = m_fieldToAttachmentPoint[f];
                Framebuffer::Attachment::Ref attachment = m_framebuffer->get(ap);
                 
                if (attachment->texture() != NULL) {
                    attachment->texture()->resize(w, h);
                } else {
                    // Renderbuffers can't be resized, so reallocate.
                    debugAssert(attachment->renderbuffer() != NULL);
                    const std::string& n = format("%s/%s", m_name.c_str(), Field(f).toString());
                    m_framebuffer->set(ap, Renderbuffer::createEmpty(n, w, h, m_specification.format[f]));
                }
            }
        }
    } else {
        // Allocate
        m_texturesAllocated = true;
        for (int f = 0; f < Field::COUNT; ++f) {
            const ImageFormat* fmt = m_specification.format[f];
            const Framebuffer::AttachmentPoint ap = m_fieldToAttachmentPoint[f];
            const std::string& n = format("%s/%s", m_name.c_str(), Field(f).toString());

            if (fmt != NULL) {
                if (GLCaps::supportsTexture(fmt)) {
                    Texture::Ref texture = Texture::createEmpty(n, w, h, fmt, Texture::DIM_2D_NPOT,
                                                                Texture::Settings::buffer());
                    m_framebuffer->set(ap, texture);

                    if (f == Field::SS_POSITION_CHANGE) {
                        texture->visualization.min = -10.0f;
                        texture->visualization.max = 10.0f;
                        //debugPrintf("SS_POSITION_CHANGE at %d\n", (int)ap - (int)Framebuffer::COLOR0);
                    } else if (f == Field::DEPTH_AND_STENCIL) {
                        texture->visualization = Texture::Visualization::depthBuffer();
                    }
                } else {
                    // We have to use a renderbuffer for this attachment
                    m_framebuffer->set(ap, Renderbuffer::createEmpty(n, w, h, fmt));
                }
            }
        }
    }
}

} // namespace G3D
