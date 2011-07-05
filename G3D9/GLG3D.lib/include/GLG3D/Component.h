/**
 \file    GLG3D/Component.h
 \author  Morgan McGuire, http://graphics.cs.williams.edu
 \created 2009-02-19
 \edited  2011-06-27
*/
#ifndef G3D_Component_h
#define G3D_Component_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "G3D/ImageFormat.h"
#include "G3D/Image1.h"
#include "G3D/Image3.h"
#include "G3D/Image4.h"
#include "G3D/SpeedLoad.h"
#include "GLG3D/Texture.h"

namespace G3D {

/** Used by Component */
enum ImageStorage {
    /** Ensure that all image data is stored exclusively on the CPU. */
    MOVE_TO_CPU, 

    /** Ensure that all image data is stored exclusively on the GPU. */
    MOVE_TO_GPU, 

    /** Ensure that all image data is stored at least on the CPU. */
    COPY_TO_CPU, 

    /** Ensure that all image data is stored at least on the GPU. */
    COPY_TO_GPU,

    /** Do not change image storage */
    IMAGE_STORAGE_CURRENT
};


class ImageUtils {
public:
    /** Returns the equivalent 8-bit version of a float format */
    static const ImageFormat* to8(const ImageFormat* f);
};

#define ReferenceCountedPointer ReferenceCountedPointer

/** Manages CPU and GPU versions of image data and performs
    conversions as needed.

    \param Image the CPU floating-point image format to use.  On the GPU, the corresponding uint8 format is used.

    Primarily used by Component. */
template<class Image>
class MapComponent : public ReferenceCountedObject {
public:

    typedef ReferenceCountedPointer<MapComponent<Image> > Ref;

private:
            
#   define MyType class MapComponent<Image>

    ReferenceCountedPointer<Image> m_cpuImage;
    Texture::Ref                   m_gpuImage;
    typename Image::StorageType    m_min;
    typename Image::StorageType    m_max;
    typename Image::ComputeType    m_mean;

    static void getTexture(const ReferenceCountedPointer<Image>& im, Texture::Ref& tex) {
            
        Texture::Dimension dim;
        if (isPow2(im->width()) && isPow2(im->height())) {
            dim = Texture::DIM_2D;
        } else {
            dim = Texture::DIM_2D_NPOT;
        }
            
        Texture::Settings settings;
        settings.wrapMode = im->wrapMode();

        tex = Texture::fromMemory
            ("Converted", im->getCArray(), im->format(),
             im->width(), im->height(), 1, ImageUtils::to8(im->format()),
             dim, settings);
    }

    static void convert(const Color4& c, Color3& v) {
        v = c.rgb();
    }

    static void convert(const Color4& c, Color4& v) {
        v = c;
    }

    static void convert(const Color4& c, Color1& v) {
        v = c.r;
    }

    inline MapComponent(const class ReferenceCountedPointer<Image>& im, const Texture::Ref& tex) : 
        m_cpuImage(im), m_gpuImage(tex), m_min(Image::StorageType::one()), m_max(Image::StorageType::zero()),
        m_mean(Image::ComputeType::zero()) {

        // Compute min, max, mean
        if (m_gpuImage.notNull() && m_gpuImage->min().isFinite()) {
            // Use previously computed data stored in the texture
            convert(m_gpuImage->min(), m_min);
            convert(m_gpuImage->max(), m_max);
            convert(m_gpuImage->mean(), m_mean);
        } else {
            bool cpuWasNull = m_cpuImage.isNull();

            if (m_cpuImage.isNull() && m_gpuImage.notNull()) {
                m_gpuImage->getImage(m_cpuImage);
            }

            if (m_cpuImage.notNull()) {
                const typename Image::StorageType* ptr = m_cpuImage->getCArray();
                typename Image::ComputeType sum = Image::ComputeType::zero();
                const int N = m_cpuImage->width() * m_cpuImage->height();
                for (int i = 0; i < N; ++i) {
                    m_min  = m_min.min(ptr[i]);
                    m_max  = m_max.min(ptr[i]);
                    sum   += typename Image::ComputeType(ptr[i]);
                }
                m_mean = sum / (float)N;
            }

            if (cpuWasNull) {
                // Throw away the CPU image to conserve memory
                m_cpuImage = NULL;
            }
        }
    }

    /** Overloads to allow conversion of Image3 and Image4 to uint8,
        since Component knows that there is only 8-bit data in the
        floats. */
    static void speedSerialize(const Image3::Ref& im, const Color3& minValue, BinaryOutput& b) {
        (void)minValue;
        // uint8x3
        b.writeUInt8('u');
        b.writeUInt8('c');
        b.writeUInt8(3);
        Image3uint8::fromImage3(im)->speedSerialize(b);
    }

    /** Overloads to allow conversion of Image3 and Image4 to uint8,
        since Component knows that there is only 8-bit data in the
        floats. */
    static void speedSerialize(const Image4::Ref& im, const Color4& minValue,  BinaryOutput& b) {
        b.writeUInt8('u');
        b.writeUInt8('c');
        if (minValue.a < 1.0f) {
            b.writeUInt8(4);
            Image4uint8::fromImage4(im)->speedSerialize(b);
        } else {
            // The alpha channel is unused, so compress this to RGB8.
            b.writeUInt8(3);
            Image3uint8::Ref im3 = Image3uint8::fromImage4(im);
            im3->speedSerialize(b);
        }
    }

    /** Overloads to allow conversion of Image3 and Image4 to uint8,
        since Component knows that there is only 8-bit data in the
        floats. 

        Note that the im is never actually used, since we don't want
        to waste time converting to float!
    */
    static void speedDeserialize(Image3::Ref& ignore, Texture::Ref& tex, const Color3& minValue, BinaryInput& b) {
        (void)minValue;

        uint8 s = b.readUInt8();
        alwaysAssertM(s == 'u', "Wrong sign value when reading Image3uint8");
        uint8 type = b.readUInt8();
        alwaysAssertM(type == 'c', "Wrong type when reading Image3uint8");
        uint8 channels = b.readUInt8();
        alwaysAssertM(channels == 3, "Wrong number of channels when reading Image3uint8");

        Image3uint8::Ref im = Image3uint8::speedCreate(b);

        Texture::Dimension dim;
        if (isPow2(im->width()) && isPow2(im->height())) {
            dim = Texture::DIM_2D;
        } else {
            dim = Texture::DIM_2D_NPOT;
        }
            
        Texture::Settings settings;
        settings.wrapMode = im->wrapMode();

        tex = Texture::fromMemory
            ("SpeedLoaded", im->getCArray(), im->format(),
             im->width(), im->height(), 1, im->format(),
             dim, settings);
    }

    /** Overloads to allow conversion of Image3 and Image4 to uint8,
        since Component knows that there is only 8-bit data in the
        floats. 
    */
    static void speedDeserialize(Image4::Ref ignore, Texture::Ref& tex, const Color4& minValue, BinaryInput& b) {
        uint8 s = b.readUInt8();
        alwaysAssertM(s == 'u', "Wrong sign value in SpeedLoad");
        uint8 type = b.readUInt8();
        alwaysAssertM(type == 'c', "Wrong type in SpeedLoad");
        uint8 channels = b.readUInt8();
        uint8 expectedChannels = (minValue.a < 1.0f) ? 4 : 3;
        alwaysAssertM(channels == expectedChannels, "Wrong number of channels when reading Image3uint8");

        if (channels == 4) {
            Image4uint8::Ref im = Image4uint8::speedCreate(b);

            Texture::Dimension dim;
            if (isPow2(im->width()) && isPow2(im->height())) {
                dim = Texture::DIM_2D;
            } else {
                dim = Texture::DIM_2D_NPOT;
            }
            
            Texture::Settings settings;
            settings.wrapMode = im->wrapMode();

            tex = Texture::fromMemory
                ("SpeedLoaded", im->getCArray(), im->format(),
                 im->width(), im->height(), 1, im->format(),
                 dim, settings);
        } else {
            alwaysAssertM(channels == 3, "Wrong number of channels");

            Image3uint8::Ref im = Image3uint8::speedCreate(b);

            Texture::Dimension dim;
            if (isPow2(im->width()) && isPow2(im->height())) {
                dim = Texture::DIM_2D;
            } else {
                dim = Texture::DIM_2D_NPOT;
            }
            
            Texture::Settings settings;
            settings.wrapMode = im->wrapMode();

            tex = Texture::fromMemory
                ("SpeedLoaded", im->getCArray(), im->format(),
                 im->width(), im->height(), 1, im->format(),
                 dim, settings);
        }
    }

    
    /** For speedCreate */
    MapComponent() {}

public:

    /** \sa SpeedLoad */
    static Ref speedCreate(BinaryInput& b) {
        Ref m = new MapComponent<Image>();
        
        m->m_min.deserialize(b);
        m->m_max.deserialize(b);
        m->m_mean.deserialize(b);

        speedDeserialize(m->m_cpuImage, m->m_gpuImage, m->m_min, b);

        return m;
    }


    /** \sa SpeedLoad */
    void speedSerialize(BinaryOutput& b) const {
        m_min.serialize(b);
        m_max.serialize(b);
        m_mean.serialize(b);

        // Save 8-bit data.  If the CPU image was NULL, we end up
        // reading it from the GPU to the CPU, converting to float,
        // and then converting back to uint8.  But we don't serialize
        // very often--this is a preprocess--so the perf hit is
        // irrelevant.

        // Don't bother saving the alpha channel if m_min == 1
        speedSerialize(image(), m_min, b);
    }

    /** Returns NULL if both are NULL */
    static Ref create
    (const class ReferenceCountedPointer<Image>& im, 
     const Texture::Ref&                         tex) {

        if (im.isNull() && tex.isNull()) {
            return NULL;
        } else {
            return new MapComponent<Image>(im, tex);
        }
    }


    /** Largest value in each channel of the image */
    const typename Image::StorageType& max() const {
        return m_max;
    }

    /** Smallest value in each channel of the image */
    const typename Image::StorageType& min() const {
        return m_min;
    }

    /** Average value in each channel of the image */
    const typename Image::ComputeType& mean() const {
        return m_mean;
    }
           
    /** Returns the CPU image portion of this component, synthesizing
        it if necessary.  Returns NULL if there is no GPU data to 
        synthesize from.*/
    const class ReferenceCountedPointer<Image>& image() const {
        MyType* me = const_cast<MyType*>(this);
        if (me->m_cpuImage.isNull()) {
            debugAssert(me->m_gpuImage.notNull());
            // Download from GPU.  This works because C++
            // dispatches the override on the static type,
            // so it doesn't matter that the pointer is NULL
            // before the call.
            m_gpuImage->getImage(me->m_cpuImage);
        }
                
        return m_cpuImage;
    }
    

    /** Returns the GPU image portion of this component, synthesizing
        it if necessary.  */
    const Texture::Ref& texture() const {
        MyType* me = const_cast<MyType*>(this);
        if (me->m_gpuImage.isNull()) {
            debugAssert(me->m_cpuImage.notNull());
                    
            // Upload from CPU
            getTexture(m_cpuImage, me->m_gpuImage);
        }
                
        return m_gpuImage;
    }

    void setStorage(ImageStorage s) const {
        MyType* me = const_cast<MyType*>(this);
        switch (s) {
        case MOVE_TO_CPU:
            image();
            me->m_gpuImage = NULL;
            break;

        case MOVE_TO_GPU:
            texture();
            me->m_cpuImage = NULL;
            break;

        case COPY_TO_GPU:
            texture();
            break;

        case COPY_TO_CPU:
            image();
            break;

        case IMAGE_STORAGE_CURRENT:
            // Nothing to do
            break;
        }
    }

#   undef MyType
};


/** \brief Common code for G3D::Component1, G3D::Component3, and G3D::Component4.

    Product of a constant and an image. 

    The image may be stored on either the GPU (G3D::Texture) or
    CPU (G3D::Map2D subclass), and both factors are optional. The
    details of this class are rarely needed to use Material, since
    it provides constructors from all combinations of data
    types.
    
    Supports only floating point image formats because bilinear 
    sampling of them is about 9x faster than sampling int formats.
    */
template<class Color, class Image>
class Component {
public:

    enum Factors {
        /** rgb() will always be zero (says nothing about the alpha value) */
        BLACK,

        /** There is no map, but there is a non-black constant specified. */
        CONSTANT,

        /** There is no constant, but there is a map specified that is assumed to be not all black. */
        MAP,

        /** There is a map and a non-black constant. */
        MAP_TIMES_CONSTANT
    };

private:

    Factors                   m_factors;
    Color                     m_constant;

    Color                     m_max;
    Color                     m_min;
    Color                     m_mean;

    /** NULL if there is no map. This is a reference so that
        multiple Components may share a texture and jointly
        move it to and from the GPU.*/
    ReferenceCountedPointer<MapComponent<Image> >  m_map;

    void init(const Color& constant) {
        m_constant = constant;

        if (constant.rgb() == Color3::zero()) {
            m_factors = BLACK;
        } else if (m_map.isNull()) {
            m_factors = CONSTANT;
        } else if (constant.rgb() == Color3::one()) {
            m_factors = MAP;
        } else {
            m_factors = MAP_TIMES_CONSTANT;
        }

        if (m_map.notNull()) {
            m_max  = m_constant * Color(m_map->max());
            m_min  = m_constant * Color(m_map->min());
            m_mean = m_constant * Color(m_map->mean());
        } else {
            m_max  = m_constant;
            m_min  = m_constant;
            m_mean = m_constant;
        }
    }

    static float alpha(const Color1& c) {
        return 1.0;
    }
    static float alpha(const Color3& c) {
        return 1.0;
    }
    static float alpha(const Color4& c) {
        return c.a;
    }


public:

    /** All zero */
    Component() : m_map(NULL) {
        init(Color::zero());
    }
        
    /** Assumes a map of NULL (all white) if not specified */
    Component
    (const Color&                    constant, 
     const ReferenceCountedPointer<MapComponent<Image> >& map = NULL) :
        m_map(map) {

        init(constant);
    }

    Component
    (const Color&                    constant, 
     const ReferenceCountedPointer<Image>& map) :
        m_map(MapComponent<Image>::create(map, NULL)) {

        init(constant);
    }

    Component
    (const Color&                    constant, 
     const Texture::Ref&             map) :
        m_map(MapComponent<Image>::create(NULL, map)) {

        init(constant);
    }

    /** Assumes a constant of 1 */
    Component(const ReferenceCountedPointer<MapComponent<Image> >& map) : m_map(map) {
        init(Color::one());
    }
    
    /** \sa SpeedLoad */
    void speedSerialize(BinaryOutput& b) const {
        SpeedLoad::writeHeader(b, "Component");

        b.writeInt32(m_factors);

        // Save the size of the color field to help ensure that
        // this was properly deserialized
        b.writeInt32(sizeof(Color));

        m_min.serialize(b);
        m_max.serialize(b);
        m_mean.serialize(b);
        
        m_constant.serialize(b);
        b.writeBool8(m_map.notNull());
        
        if (m_map.notNull()) {
            m_map->speedSerialize(b);
        }
    }


    /** \sa SpeedLoad */
    void speedDeserialize(BinaryInput& b) {
        SpeedLoad::readHeader(b, "Component");

        m_factors = (Factors)b.readInt32();
        
        const size_t colorSize = (size_t)b.readInt32();
        alwaysAssertM(colorSize == sizeof(Color), 
                      "Tried to SpeedLoad a component in the wrong format.");

        m_min.deserialize(b);
        m_max.deserialize(b);
        m_mean.deserialize(b);

        m_constant.deserialize(b);
        bool hasMap = b.readBool8();
        if (hasMap) {
            m_map = MapComponent<Image>::speedCreate(b);
        }
    }

    bool operator==(const Component<Color, Image>& other) const {
        return 
            (m_factors == other.m_factors) &&
            (m_constant == other.m_constant) &&
            (m_map == other.m_map);
    }
    
    Factors factors() const {
        return m_factors;
    }
        
    /** Return constant * map.  Optimized to only perform as many
        operations as needed.

        If the component contains a texture map that has not been
        converted to a CPU image, that conversion is
        performed. Because that process is not threadsafe, when using
        sample() in a multithreaded environment, first invoke setStorage(COPY_TO_CPU)
        on every Component from a single thread to prime the CPU data
        structures.

        Coordinates are normalized; will be scaled by the image width and height
        automatically.
    */
    Color sample(const Vector2& pos) const {
        switch (m_factors) {
        case BLACK:
        case CONSTANT:
            return m_constant;

        case MAP:
            {
                const typename Image::Ref& im = m_map->image();
                return im->bilinear(pos * Vector2(im->width(), im->height()));
            }
        case MAP_TIMES_CONSTANT:
            {
                const typename Image::Ref& im = m_map->image();
                return im->bilinear(pos * Vector2(im->width(), im->height())) * m_constant;
            }

        default:
            alwaysAssertM(false, "fell through switch");
            return Color::zero();
        }
    }
        
    inline const Color& constant() const {
        return m_constant;
    }

    /** Largest value per color channel */
    inline const Color& max() const {
        return m_max;
    }

    /** Smallest value per color channel */
    inline const Color& min() const {
        return m_min;
    }

    /** Average value per color channel */
    inline const Color& mean() const {
        return m_mean;
    }

    /** Causes the image to be created by downloading from GPU if necessary. 
        Returns NULL if the underlying map is NULL.*/
    inline const ReferenceCountedPointer<Image>& image() const {
        if (m_map.isNull()) {
            static const ReferenceCountedPointer<Image> n;
            return n;
        } else {
            return m_map->image();
        }
    }

    /** Causes the texture to be created by uploading from CPU if necessary. 
        Returns NULL if the underlying map is NULL.*/
    inline const Texture::Ref& texture() const {
        if (m_map.isNull()) {
            static const Texture::Ref n;
            return n;
        } else {
            return m_map->texture();
        }
    }

    /** Does not change storage if the map is NULL */
    inline void setStorage(ImageStorage s) const {
        if (m_map.notNull()) {
            m_map->setStorage(s);
        }
    }

    /** Says nothing about the alpha channel */
    inline bool notBlack() const {
        return factors() != BLACK;
    }

    /** Returns true if there is non-unit alpha. */
    inline bool nonUnitAlpha() const {
        return (alpha(m_min) != 1.0f);
    }

    /** Says nothing about the alpha channel */
    inline bool isBlack() const {
        return factors() == BLACK;
    }
};

typedef Component<Color1, Image1> Component1;

typedef Component<Color3, Image3> Component3;

typedef Component<Color4, Image4> Component4;

}

#endif
