#ifndef G3D_GBuffer_h
#define G3D_GBuffer_h

namespace G3D {

class GBuffer {
public:

    class Settings {
    public:
        /** Generate per-face normal buffer in camera space */
        bool      csFaceNormals;

        /** Generate interplated vertex normal buffer in camera space */
        bool      csShadeNormals;

        /** Generate packed linear camera-space z buffer */
        bool      csLinearZ;

        /** May only be enabled if format is a floating point format */
        bool      csPosition;

        /** Generate traditional OpenGL depth buffer */
        bool      depth;

        // TODO: other fields based on the Material design

        /** If a fixed point (i.e., integer) format is used, normals
            are packed as 2*n-1 and linear z is packed according to
            the available precision of that format.

            If a floating point format is used, normals are not packed
            and linear z is stored directly in the R component.
        */
        const ImageFormat*  format; 
        const ImageFormat* depthFormat;

    };


    // ...

    const Camera& camera();
        
};

} // namespace G3D

#endif

