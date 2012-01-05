/**
  \file GLG3D/Texture2.h
 
  Copyright 2000-2012, Morgan McGuire.
  All rights reserved.
 */

#ifndef GLG3D_TEXTURE2_H
#define GLG3D_TEXTURE2_H

#include "G3D/ImageBuffer.h"
#include "G3D/WrapMode.h"


namespace G3D {

/**
    Replacement for Texture using ImageBuffer class.
*/
class Texture2 {
public:
    typedef ReferenceCountedPointer<class Texture2> Ref;

    /** 
        General type for OpenGL target of texture.
        Support for NPOT textures is assumed removing the need for rectangle targets. */
    enum Dimension {
        DIM_2D,
        DIM_3D,
        DIM_CUBE
    };

    /**
        Trilinear mipmap is the best quality (and frequently fastest)
        mode.  The no-mipmap modes conserve memory.  Non-interpolating
        ("Nearest") modes are generally useful only when packing lookup
        tables into textures for shaders.

        3D textures do not support mipmap interpolation modes. */
    enum InterpolateMode {
        TRILINEAR_MIPMAP = 3, 
        BILINEAR_MIPMAP = 4,
        NEAREST_MIPMAP = 5,

        BILINEAR_NO_MIPMAP = 2,
        NEAREST_NO_MIPMAP = 6
	};

    /** 
        A depth texture can automatically perform the depth
        comparison used for shadow mapping on a texture lookup.  The
        result of a texture lookup is thus the shadowed amount (which
        will be percentage closer filtered on newer hardware) and
        <I>not</I> the actual depth from the light's point of view.
       
        This combines GL_TEXTURE_COMPARE_MODE_ARB and GL_TEXTURE_COMPARE_FUNC_ARB from
        http://www.nvidia.com/dev_content/nvopenglspecs/GL_ARB_shadow.txt

        For best results on percentage closer hardware (GeForceFX and Radeon9xxx or better), 
        create shadow maps as depth textures with BILINEAR_NO_MIPMAP sampling.

        See also G3D::RenderDevice::configureShadowMap and the Collision_Demo. */
    enum DepthReadMode {
        DEPTH_NORMAL = 0,
        DEPTH_LEQUAL = 1,
        DEPTH_GEQUAL = 2
    };

    /** 
        All parameters of a texture that are independent of the
        underlying image data. */
    class Settings {
    public:
        /** Default is TRILINEAR_MIPMAP */
        InterpolateMode             interpolateMode;

        /** Default is TILE */
        WrapMode                    wrapMode;

        /** Default is DEPTH_NORMAL */
        DepthReadMode               depthReadMode;

        /** Default is 2.0 */
        float                       maxAnisotropy;

        /** Default is true */
        bool                        autoMipMap;

        /**
         Highest MIP-map level that will be used during rendering.
         The highest level that actually exists will be L =
         log(max(m_width, m_height), 2)), although it is fine to set
         maxMipMap higher than this.  Must be larger than minMipMap.
         Default is 1000.

         Setting the max mipmap level is useful for preventing
         adjacent areas of a texture from being blurred together when
         viewed at a distance.  It may decrease performance, however,
         by forcing a larger texture into cache than would otherwise
         be required.
         */
        int                         maxMipMap;

        /**
         Lowest MIP-map level that will be used during rendering.
         Level 0 is the full-size image.  Default is -1000, matching
         the OpenGL spec.  @cite
         http://oss.sgi.com/projects/ogl-sample/registry/SGIS/texture_lod.txt
         */
        int                         minMipMap;

        Settings();

        /** \param any Must be in the form of a table of the fields or appear as
            a call to a static factory method, e.g.,:

            - Texture::Settings{  interpolateMode = "TRILINEAR_MIPMAP", wrapMode = "TILE", ... }
            - Texture::Settings::video()
        */
        Settings(const Any& any);

        Any toAny() const;

        static const Settings& defaults();

        /** 
          Useful defaults for video/image processing.
          BILINEAR_NO_MIPMAP / CLAMP / DEPTH_NORMAL / 1.0 / automipmap =false
        */
        static const Settings& video();

        /** 
          Useful defaults for general purpose computing.
          NEAREST_NO_MIPMAP / CLAMP / DEPTH_NORMAL / 1.0 / false
        */
        static const Settings& buffer();

        /** 
          Useful defaults for shadow maps.
          BILINEAR_NO_MIPMAP / CLAMP / DEPTH_LEQUAL / 1.0 / false
        */
        static const Settings& shadow();

        /**
           Useful defaults for cube maps
           TRILINEAR_MIPMAP / CLAMP, DEPTH_NORMAL / 1.0 / true 
         */
        static const Settings& cubeMap();

        bool operator==(const Settings& other) const;

        /** True if both Settings are identical, ignoring mipmap settings.*/
        bool equalsIgnoringMipMap(const Settings& other) const;
        size_t hashCode() const;
    };

    class Specification {
    public:
        std::string               filename;

        /** Defaults to ImageFormat::AUTO() */
        const class ImageFormat*  desiredFormat;

        /** Defaults to Texture::DIM_2D */
        Dimension                 dimension;

        Settings                  settings;

        Specification() : desiredFormat(ImageFormat::AUTO()), 
                          dimension(DIM_2D) {}

        Specification(const Any& any);

        void deserialize(BinaryInput& b);

        Specification(BinaryInput& b) {
            deserialize(b);
        }

        bool operator==(const Specification& s) const;

        bool operator!=(const Specification& s) const {
            return !(*this == s);
        }

        Any toAny() const;

        void serialize(BinaryOutput& b) const;
    };

    static const char* toString(Dimension m);
    static Dimension toDimension(const std::string& s);

    static const char* toString(InterpolateMode m);
    static InterpolateMode toInterpolateMode(const std::string& s);

    static const char* toString(DepthReadMode m);
    static DepthReadMode toDepthReadMode(const std::string& s);


    static Ref create(const Specification& s);

    /**
        Creates a Texture from an ImageBuffer.  Does not keep a reference to the buffer. */
    static Texture2::Ref fromImageBuffer(
        const std::string&              name,
        const ImageBuffer::Ref&         image,
        const ImageFormat*              desiredFormat  = ImageFormat::AUTO(),
        Dimension                       dimension      = DIM_2D,
        const Settings&                 settings       = Settings::defaults());

private:
    typedef Array< Array<ImageBuffer::Ref> > MipsPerCubeFace;

    bool validateSettings();
    void configureTexture(const MipsPerCubeFace& mipsPerCubeFace);
    void uploadImages(const MipsPerCubeFace& mipsPerCubeFace);

    Texture2(
        const std::string&          name,
        const MipsPerCubeFace&      mipsPerCubeFace,
        Dimension                   dimension,
        InterpolateMode             interpolation,
        WrapMode                    wrapping,
        const ImageFormat*          format,
        const Settings&             settings); 
};

} // namespace G3D

#endif // GLG3D_TEXTURE2_H