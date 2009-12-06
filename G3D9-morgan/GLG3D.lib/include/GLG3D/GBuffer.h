#ifndef G3D_GBuffer_h
#define G3D_GBuffer_h

namespace G3D {

class GBuffer {
public:

    /** All positions and directions are generated in camera space. */
    enum BufferType {
        /** Generate per-face normal buffer in camera space. For
         floating point formats, encoded as RGB=xyz.  For GL_BYTE
         formats, encoded as <code>RGB=(xyz + 1) / 2</code>.*/
        FACE_NORMALS,

        /** Generate interplated vertex normal buffer in camera space.
            For floating point formats, encoded as RGB=xyz.  For
            GL_BYTE formats, encoded as RGB=(xyz + 1) / 2. */
        SMOOTH_NORMALS,

        /** Generate linear camera-space z buffer, encoded as 

            <code>RGBA = vec4(1.0f, 256.0, 65536.0, 16777216.0) * z</code>.

            The unpack code is:

            <code>z = dot(RGBA, vec4(1.0/1.0, 1.0/256.0, 1.0/65536.0, 1.0/16777216.0))</code>.

            (if you want linear Z for a float format, just read csPosition.z)

            \cite http://http.developer.nvidia.com/GPUGems/gpugems_ch12.html
        */
        Z,

        /** May only be enabled if format is a floating point format. */
        POSITION,

        /** RGB = lambertian RGB, A = clear coat magnitude. For fast
            deferred shading without transmission or emission. */
        LAMBERTIAN,

        /** RGB = glossy RGB, A = SuperBSDF shininess parameter. For
            fast deferred shading without transmission or emission. */
        GLOSSY,

        /** RGB = emitted radiance, A = reserved. For fast emission
            without other deferred shading.*/
        EMIT,

        /** SuperBSDF lambertian parameters */
        SS_0

        /** SuperBSDF glossy parameters */
        SS_1,

        /** R = SuperBSDF emit, G = SuperBSDF transmit, B = absorption
            coefficient, A = index of refraction */
        SS_2
    };

    /** Converts the enum to a human-readable */
    static const char* toString(BufferType t);

    /** Converts a string to a BufferType. */
    static BufferType bufferType(const std::string&);

    class Settings {
    public:

        /**  If more buffers are requested than can be rendered in one
            pass (using multiple draw buffers), rendering will occur
            in multiple passes. */
        Set<BufferType>     buffers;

        const ImageFormat*  format; 

        /** A depth buffer is always available. */
        const ImageFormat*  depthFormat;
    };

private:

    class Pass {
    public:
        Array<BufferType>    bufferTypeArray;
        FrameBuffer::Ref     frameBuffer;
        Shader::Ref          shader;
    };

    Settings                 m_settings;

    int                      m_width;

    int                      m_height;

    /** Last camera used */
    Camera                   m_camera;

    /** Maximum number of buffers that can be rendered in a single pass */
    const int                m_buffersPerPass;

    Table<BufferType, Texture::Ref> m_texture;
    Texture::Ref             m_depth;
    Array<Pass>              m_passArray;

public:

    static Ref create(int width, int height, const Settings& settings);

    void resize(int width, int height);

    const Settings& settings() const;

    const Camera& camera() const;

    inline int width() const {
        return m_width;
    }

    inline int height() const {
        return m_height;
    }

    /**
       \param surfaceArray Non-SuperSurface elements will be ignored.
     */
    void update
    (RenderDevice*                    rd, 
     const Camera&                    camera,
     const Array<Surface::Ref>&       surfaceArray);

    Texture::Ref operator[](BufferType b) const;

};

} // namespace G3D

#endif

