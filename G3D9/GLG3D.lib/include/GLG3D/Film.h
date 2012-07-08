/**
 \file GLG3D/Film.h
 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2008-07-01
 \edited  2011-07-08
 */

#ifndef G3D_Film_h
#define G3D_Film_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/Shader.h"
#include "GLG3D/Framebuffer.h"
#include "GLG3D/Texture.h"
#include "GLG3D/GuiContainer.h"

namespace G3D {

/** \brief Post processing: gamma correction, exposure, bloom, and
    screen-space antialiasing.

   Computer displays are not capable of representing the range of
   values that are rendered by a physically based system.  For
   example, the brightest point on a monitor rarely has the intensity
   of a light bulb.  Furthermore, for historical (and 2D GUI
   rendering) reasons, monitors apply a power ("gamma") curve to
   values.  So rendering code that directly displays radiance values
   on a monitor will neither capture the desired tonal range nor even
   present the values scaled linearly.  The Film class corrects for
   this using the simple tone mapping algorithm presented in Pharr and
   Humphreys 2004 extended with color desaturation.

   To use, render to a G3D::Texture using G3D::Framebuffer, then pass that
   texture to exposeAndDraw() to produce the final image for print or display
   on screen. For example, on initialization, perform:

   <pre>
    film = Film::create();
    fb = Framebuffer::create("Offscreen");
    colorBuffer = Texture::createEmpty("Color", renderDevice->width(), renderDevice->height(), ImageFormat::RGB16F(), Texture::DIM_2D_NPOT, Texture::Settings::video());
    fb->set(Framebuffer::COLOR_ATTACHMENT0, colorBuffer);
    fb->set(Framebuffer::DEPTH_ATTACHMENT, 
        Texture::createEmpty("Depth", renderDevice->width(), renderDevice->height(), ImageFormat::DEPTH24(), Texture::DIM_2D_NPOT, Texture::Settings::video()));
   </pre>

   and then per frame,
   <pre>
    rd->pushState(fb);
        ...Rendering code here...
    rd->popState(); 
    film->exposeAndRender(rd, colorBuffer);
   </pre>

   The bloom effects are most pronounced when rendering values that are 
   actually proportional to radiance.  That is, if all of the values in the
   input are on a narrow range, there will be little bloom.  But if the 
   sky, highlights, emissive surfaces, and light sources are 10x brighter 
   than most scene objects, they will produce attractive glows and halos.

   When rendering multiple viewports or off-screen images, use a separate 
   Film instance for each size of input for maximum performance.

   Requires shaders.
*/
class Film : public ReferenceCountedObject {
public:

    typedef ReferenceCountedPointer<class Film> Ref;

private:

    /** Used for all buffers except m_postGamma */
    const ImageFormat*      m_intermediateFormat;

    /** Used for m_postGamma */
    const ImageFormat*      m_targetFormat;

    /** Working framebuffer */
    Framebuffer::Ref        m_framebuffer;
    Framebuffer::Ref        m_tempFramebuffer;
    Framebuffer::Ref        m_blurryFramebuffer;

    /** Used to make the last step of gamma write to the m_postGamma
     texture instead of the current framebuffer.*/
    Framebuffer::Ref        m_postGammaFramebuffer;

    /** Post-bloom, pre-AA texture. */
    Texture::Ref            m_postGamma;

    /** Performs screen-space antialiasing */
    Shader::Ref             m_antialiasingShader;

    /** Expose, invert gamma and correct out-of-gamut colors */
    Shader::Ref             m_shader;

    /** Expose before bloom */
    Shader::Ref             m_preBloomShader;

    /** Output of blend shader/input to the vertical blur. 16-bit float.*/
    Texture::Ref            m_blended;

    /** float pre-bloom curve applied */
    Texture::Ref            m_preBloom;

    /** float blurred vertical */
    Texture::Ref            m_temp;

    /** float blurred vertical + horizontal */
    Texture::Ref            m_blurry;

    /** \brief Monitor gamma used in tone-mapping. Default is 2.0. */
    float                   m_gamma;

    /** \brief Scale factor applied to the pixel values during exposeAndRender(). */
    float                   m_sensitivity;

    /** \brief 0 = no bloom, 1 = blurred out image. */
    float                   m_bloomStrength;

    /** \brief Bloom filter kernel radius as a fraction 
     of the larger of image width/height.*/
    float                   m_bloomRadiusFraction;

    bool                    m_antialiasingEnabled;

    float                   m_vignetteTopStrength;
    float                   m_vignetteBottomStrength;
    float                   m_vignetteSizeFraction;

    /** Loads the shaders. Called from expose. */
    void init();

    Film(const ImageFormat* f, const ImageFormat* t);

public:
    
    /** \brief Create a new Film instance.
    
        \param intermediateFormat Intermediate precision to use when processing images during bloom.  This should usually
        match your source format. Defaults to 
         ImageFormat::RGB16F. A floating-point texture is used in case values are not on the range (0, 1).
         If you know that your data is on a smaller range, try ImageFormat::RGB8() or 
         ImageFormat::RGB10A2() for increased space savings or performance.

         \param targetFormat Intermediate precision used when processing images after bloom, during antialiasing.
         This should usually match your destination format.  Defaults ot ImageFormat::RGB8().
      */
    static Ref create(const ImageFormat* intermediateFormat = ImageFormat::RGB16F(), 
                      const ImageFormat* targetFormat = ImageFormat::RGB8());

    /** Amount of darkness due to vignetting for the top of the screen, on the range [0, 1] */
    float vignetteTopStrength() const {
        return m_vignetteTopStrength;
    }

    void setVignetteTopStrength(float s) {
        m_vignetteTopStrength = s;
    }
    
    void setVignetteBottomStrength(float s) {
        m_vignetteBottomStrength = s;
    }

    void setVignetteSizeFraction(float s) {
        m_vignetteSizeFraction = s;
    }

    /** Amount of darkness due to vignetting for the bottom of the screen, on the range [0, 1] */
    float vignetteBottomStrength() const {
        return m_vignetteBottomStrength;
    }

    /** Fraction of the diagonal radius of the screen covered by vignette, on the range [0, 1] */
    float vignetteSizeFraction() const {
        return m_vignetteSizeFraction;
    }

    /** \brief Monitor gamma used in tone-mapping. Default is 2.0. */
    float gamma() const {
        return m_gamma;
    }

    /** \brief Scale factor applied to the pixel values during exposeAndRender() */
     float sensitivity() const {
        return m_sensitivity;
    }

    /** \brief 0 = no bloom, 1 = blurred out image. */
    float bloomStrength() const {
        return m_bloomStrength;
    }

    /** \brief Bloom filter kernel radius as a fraction 
     of the larger of image width/height.*/
    float bloomRadiusFraction() const {
        return m_bloomRadiusFraction;
    }

    /** Enabled antialiasing post-processing. This reduces the
     artifacts from undersampling edges but may blur textures.
    By default, this is disabled.

    The current antialiasing algorithm is "FXAA 1" by Timothy Lottes.
    This may change in future implementations.
    */
    void setAntialiasingEnabled(bool e) {
        m_antialiasingEnabled = e;
    }

    bool antialiasingEnabled() const {
        return m_antialiasingEnabled;
    }

    void setGamma(float g) {
        m_gamma = g;
    }

    void setSensitivity(float s) {
        m_sensitivity = s;
    }

    void setBloomStrength(float s) {
        m_bloomStrength = s;
    }

    void setBloomRadiusFraction(float f) {
        m_bloomRadiusFraction = f;
    }

    /** Adds controls for this Film to the specified GuiPane. */
    void makeGui
    (class GuiPane*, 
     float maxSensitivity = 10.0f, 
     float sliderWidth    = GuiContainer::CONTROL_WIDTH, 
     float controlIndent  = 0.0f);

    /** \brief Renders the input as filtered by the film settings to the currently bound framebuffer.
        \param downsample One side of the downsampling filter in pixels. 1 = no downsampling. 2 = 2x2 downsampling (antialiasing). Not implemented.

        If rendering to a bound texture, set the Texture::Visualization::documentGamma = gamma() afterwards.
    */
    void exposeAndRender
        (RenderDevice* rd, 
         const Texture::Ref& input,
         int downsample = 1);

    /**
      Render to texture helper.  You can also render to a texture by binding \a output to a FrameBuffer, 
      setting the FrameBuffer on the RenderDevice, and calling the 
      three-argument version of exposeAndRender.  That process will be faster than this
      version, which must create its FrameBuffer every time it is invoked.

     \param output If NULL, this will be allocated to be the same size and format as \a input.
     */
    void exposeAndRender
        (RenderDevice*      rd,
        const Texture::Ref& input,
        Texture::Ref&       output,
        int                 downsample = 1);
};

} // namespace
#endif
