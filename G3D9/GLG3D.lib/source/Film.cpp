/**
 \file GLG3D.lib/source/Film.cpp
 \author Morgan McGuire, http://graphics.cs.williams.edu
 */

#include "GLG3D/Film.h"
#include "G3D/debugAssert.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/GaussianBlur.h"
#include "GLG3D/Draw.h"
#include "G3D/fileutils.h"

namespace G3D {

/** 
Prepend #version and #define BLOOM to alter the optimization strategy
*/
static const char* shaderCode = 
"uniform sampler2D sourceTexture;\n\
#ifdef BLOOM\n\
    uniform sampler2D bloomTexture;\n\
    uniform float     bloomStrengthScaled;\n\
#endif\n\
uniform float     sensitivity;\
\
/* 1.0 / monitorGamma.  Usually about invGamma = 0.45*/\
uniform float     invGamma;\
\n\
#if __VERSION__ < 150\n\
#define texelFetch texelFetch2D\n\
#endif\n\
void main(void) {\
\n\
    vec3 src   = texelFetch(sourceTexture, ivec2(gl_TexCoord[0].st * g3d_sampler2DSize(sourceTexture)), 0).rgb;\n\
\
    src *= sensitivity;\n\
#   ifdef BLOOM\n\
        vec3 bloom = texture2D(bloomTexture,  gl_TexCoord[0].st).rgb;\n\
        src += bloom * bloomStrengthScaled;\n\
#   endif\n\
\
    /* Fix out-of-gamut saturation*/\
    /* Maximumum channel:*/\
    float m = max(max(src.r, src.g), max(src.b, 1.0));\
    /* Normalized color*/\
    src *= (1.0 / m);\
    /* Fade towards white when the max is brighter than 1.0 (like a light saber core)*/\
    src = mix(src, vec3(1.0), clamp((m - 1.0) * 0.2, 0.0, 1.0));\
    \
    /* Invert the gamma curve */\
    vec3 dst = pow(src, vec3(invGamma));\
    \
    gl_FragColor.rgb = dst;\
}";

static const char* preBloomShaderCode = 
"uniform sampler2D sourceTexture;\
uniform float     sensitivity;\
\n\
#if __VERSION__ < 150\n\
#define texelFetch texelFetch2D\n\
#endif\n\
void main(void) {\n\
    vec3 src = texelFetch(sourceTexture, ivec2(gl_TexCoord[g3d_Index(sourceTexture)].st * g3d_sampler2DSize(sourceTexture)), 0).rgb * sensitivity;\n\
    float p  = max(max(src.r, src.g), src.b);\
    gl_FragColor.rgb = src * smoothstep(1.0, 2.0, p);\
}";


    /*
static const char* antialiasingShaderCode =
STR(
uniform sampler2D sourceTexture;
void main() {
    gl_FragColor.rgb = texture2D(sourceTexture, gl_TexCoord[g3d_Index(sourceTexture)].st).ggg;
}
);
    */

Film::Film(const ImageFormat* f, const ImageFormat* t) :
    m_intermediateFormat(f),
    m_targetFormat(t),
    m_gamma(2.2f),
    m_sensitivity(1.0f),
    m_bloomStrength(0.18f),
    m_bloomRadiusFraction(0.008f),
    m_antialiasingEnabled(false) {

    init();
}


Film::Ref Film::create(const ImageFormat* f, const ImageFormat* t) {
    return new Film(f, t);
}


void Film::init() {
    debugAssertM(m_framebuffer.isNull(), "init called twice");

    debugAssertGLOk();
    m_framebuffer       = Framebuffer::create("Film");
    m_blurryFramebuffer = Framebuffer::create("Film::m_blurryFramebuffer");
    m_tempFramebuffer   = Framebuffer::create("Film::m_tempFramebuffer");
    m_postGammaFramebuffer   = Framebuffer::create("Film::m_postGammaFramebuffer");

    // Cache shader
    static WeakReferenceCountedPointer<Shader> commonShader, commonPreBloomShader, commonAntialiasingShader;

    m_shader             = commonShader.createStrongPtr();
    m_preBloomShader     = commonPreBloomShader.createStrongPtr();
    m_antialiasingShader = commonAntialiasingShader.createStrongPtr();

    if (m_shader.isNull()) {
        alwaysAssertM(GLCaps::glslVersion() >= 1.20, 
                      format("Film requires GLSL 1.20 or later.  This GPU has version %5.2f\n",
                             GLCaps::glslVersion()));
        const std::string& version = (GLCaps::glslVersion() >= 1.50) ? 
            "#version 150 compatibility\n" : "#version 120\n#extension GL_EXT_gpu_shader4 : enable\n";

        commonShader = m_shader = Shader::fromStrings("", version + "#define BLOOM\n" + std::string(shaderCode));
        m_shader->setPreserveState(false);

        commonPreBloomShader = m_preBloomShader = Shader::fromStrings("", version + preBloomShaderCode);
        m_preBloomShader->setPreserveState(false);

        const std::string& antialiasingShaderCode = readWholeFile(System::findDataFile("Film_FXAA.pix"));
        commonAntialiasingShader = m_antialiasingShader = Shader::fromStrings("", version + antialiasingShaderCode);
        m_antialiasingShader->setPreserveState(false);
    }

}


void Film::exposeAndRender
    (RenderDevice*      rd,
    const Texture::Ref& input,
    Texture::Ref&       output,
    int                 downsample) {

    if (output.isNull()) {
        // Allocate new output texture
        output = Texture::createEmpty("Exposed image", input->width(), input->height(), input->format(), 
                                      input->dimension(), Texture::Settings::buffer());
    }

    Framebuffer::Ref fb = Framebuffer::create("Film temp");
    fb->set(Framebuffer::COLOR0, output);
    rd->push2D(fb); {
        rd->clear();
        exposeAndRender(rd, input, downsample);
    } rd->pop2D();

    // Override the document gamma
    output->visualization = Texture::Visualization::sRGB();
    output->visualization.documentGamma = gamma();
}


void Film::exposeAndRender(RenderDevice* rd, const Texture::Ref& input, int downsample) {
    debugAssertM(downsample == 1, "Downsampling not implemented in this release");
    if (m_framebuffer.isNull()) {
        init();
    }

    const int w = input->width();
    const int h = input->height();

    int blurDiameter = iRound(m_bloomRadiusFraction * 2.0f * max(w, h));
    if (isEven(blurDiameter)) {
        ++blurDiameter;
    }
    
    // Blur diameter for the vertical blur (at half resolution)
    int halfBlurDiameter = blurDiameter / 2;
    if (isEven(halfBlurDiameter)) {
        ++halfBlurDiameter;
    }

    float bloomStrength = m_bloomStrength;
    if (halfBlurDiameter <= 1) {
        // Turn off bloom; the filter radius is too small
        bloomStrength = 0;
    }

    // Allocate intermediate buffers, perhaps because the input size is different than was previously used.
    if (m_temp.isNull() || (m_temp->width() != w/2) || (m_temp->height() != h/2)) {
        // Make smaller to save fill rate, since it will be blurry anyway
        m_preBloom = Texture::createEmpty("Film::m_preBloom", w,   h,  m_intermediateFormat, Texture::defaultDimension(), Texture::Settings::video());
        m_temp     = Texture::createEmpty("Film::m_temp",    w/2, h/2, m_intermediateFormat, Texture::defaultDimension(), Texture::Settings::video());
        m_blurry   = Texture::createEmpty("Film::m_blurry",  w/4, h/4, m_intermediateFormat, Texture::defaultDimension(), Texture::Settings::video());
        
        // This may need to be resized if there is cropping for the target
        m_postGamma = Texture::createEmpty("Film::m_postGamma", w, h, m_targetFormat, Texture::defaultDimension(), Texture::Settings::video());

        // Clear the newly created textures
        m_preBloom->clear(CubeFace::POS_X, 0, rd);
        m_temp->clear(CubeFace::POS_X, 0, rd);
        m_blurry->clear(CubeFace::POS_X, 0, rd);
        m_postGamma->clear(CubeFace::POS_X, 0, rd);

        m_framebuffer->set(Framebuffer::COLOR0, m_preBloom);
        m_tempFramebuffer->set(Framebuffer::COLOR0, m_temp);
        m_blurryFramebuffer->set(Framebuffer::COLOR0, m_blurry);
        m_postGammaFramebuffer->set(Framebuffer::COLOR0, m_postGamma);
    }

    rd->push2D(); {

        // Bloom
        if (bloomStrength > 0) {
            Framebuffer::Ref oldFB = rd->drawFramebuffer();
            rd->setFramebuffer(m_framebuffer);
            rd->clear();
            m_preBloomShader->args.set("sourceTexture",  input);
            m_preBloomShader->args.set("sensitivity",    m_sensitivity);
            rd->setShader(m_preBloomShader);
            Draw::fastRect2D(m_preBloom->rect2DBounds(), rd);

            // TODO: eliminate the prebloom pass and roll it into the horizontal blur

            rd->setFramebuffer(m_tempFramebuffer);
            rd->clear();
            // Blur vertically
            GaussianBlur::apply(rd, m_preBloom, Vector2(0, 1), blurDiameter, m_temp->vector2Bounds());

            // Blur horizontally
            rd->setFramebuffer(m_blurryFramebuffer);
            rd->clear();
            GaussianBlur::apply(rd, m_temp, Vector2(1, 0), halfBlurDiameter, m_blurry->vector2Bounds());

            rd->setFramebuffer(oldFB);
        }

        if (m_antialiasingEnabled) {
            // TODO: ensure that the target has the right size
            rd->push2D(m_postGammaFramebuffer);
        }

        {
            // Combine, fix saturation, gamma correct and draw
            m_shader->args.set("sourceTexture",  input);
            m_shader->args.set("bloomTexture",   (bloomStrength > 0) ? m_blurry : Texture::zero());
            m_shader->args.set("bloomStrengthScaled",  bloomStrength * 10.0);
            m_shader->args.set("sensitivity",    m_sensitivity);
            m_shader->args.set("invGamma",       1.0f / m_gamma);
            rd->setShader(m_shader);

            Draw::fastRect2D(input->rect2DBounds(), rd);
        }

        if (m_antialiasingEnabled) {
            // Unbind the m_postGammaFramebuffer
            rd->pop2D();

            m_antialiasingShader->args.set("sourceTexture", m_postGamma);
            rd->setShader(m_antialiasingShader);
            Draw::fastRect2D(m_postGamma->rect2DBounds(), rd);
        }

    } rd->pop2D();
}


void Film::makeGui(class GuiPane* pane, float maxSensitivity, float sliderWidth, float indent) {
    GuiNumberBox<float>* n = NULL;

    n = pane->addNumberBox("Gamma",         &m_gamma, "", GuiTheme::LOG_SLIDER, 1.0f, 7.0f, 0.1f);
    n->setWidth(sliderWidth);  n->moveBy(indent, 0);

    n = pane->addNumberBox("Sensitivity",   &m_sensitivity, "", GuiTheme::LOG_SLIDER, 0.001f, maxSensitivity);
    n->setWidth(sliderWidth);  n->moveBy(indent, 0);

    n = pane->addNumberBox("Bloom Str.",    &m_bloomStrength, "", GuiTheme::LOG_SLIDER, 0.0f, 1.0f);
    n->setWidth(sliderWidth);  n->moveBy(indent, 0);

    n = pane->addNumberBox("Bloom Radius",  &m_bloomRadiusFraction, "", GuiTheme::LOG_SLIDER, 0.0f, 0.2f);
    n->setWidth(sliderWidth);  n->moveBy(indent, 0);

    GuiCheckBox* c = pane->addCheckBox("Post-process Antialiasing", &m_antialiasingEnabled);
    c->moveBy(indent, 0);
}

}
