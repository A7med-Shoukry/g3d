/**
 \file GLG3D/Args.h
  
 \maintainer Morgan McGuire, Michael Mara http://graphics.cs.williams.edu
 
 \created 2012-06-16
 \edited 2012-06-16
 */

#ifndef G3D_Args_h
#define G3D_Args_h


#include "G3D/platform.h"
#include "G3D/constants.h"
#include "G3D/SmallArray.h"
#include "GLG3D/Texture.h"
#include "GLG3D/VertexRange.h"
#include "GLG3D/glheaders.h"

namespace G3D {

/** All arguments */
class Args {

    /* 8-byte storage used for all argument types */
    union Scalar {
        float  f;
        int32  i;
        uint32 ui;
        bool   b;
        double d;
        Scalar(float fl)    { f = fl; }
        Scalar(int32 in)    { i = in; }
        Scalar(uint32 uin)  { ui = uin; }
        Scalar(bool bo)     { b = bo; }
        Scalar(double db)   { d = db; }
        Scalar()            {  }
    }; 

    /* This contains the value of a uniform or macro argument passed to shader and its type 
        Macro variables can only be a subset of the possible values */
    class Arg {
    public:
        GLenum                  type;

        // Empty unless this argument is an OpenGL Sampler
        Texture::Ref            texture;
        SmallArray<Scalar, 4>   value;
        std::string toString() const;

    };

    typedef Table<std::string, Arg> ArgTable;
       
private:
        
    std::string                                 m_preamble;
    ArgTable                                    m_macroArgs;
    ArgTable                                    m_uniformArgs;
        
    Table<std::string, const VertexRange>       m_streamArgs;
    int                                         m_numInstances;

    /** If empty, sequential indicies will be used */
    VertexRange                                 m_indexArray;

public:

    PrimitiveType geometryInput;
    PrimitiveType geometryOutput;

    void setIndexArray(const VertexRange indArray);

    std::string getPreambleAndMacroString() const;

    /** Arbitrary string to append to beginning of the shader */
    void setPreamble(const std::string& preamble);
    void setStream(const std::string& name, const VertexRange& val);
    void setStream(GLenum name, const VertexRange& val);

    void setNumInstances(int num);


    void setMacro(const std::string& name, Arg arg);

    // Supports bool, int, float, vec234, mat234
    void setMacro(const std::string& name, bool             val, bool optional = false);
    void setMacro(const std::string& name, int              val, bool optional = false);
    void setMacro(const std::string& name, uint32           val, bool optional = false);
    void setMacro(const std::string& name, double           val, bool optional = false);
    void setMacro(const std::string& name, float            val, bool optional = false);
    void setMacro(const std::string& name, const Vector2&   val, bool optional = false);
    void setMacro(const std::string& name, const Vector3&   val, bool optional = false);
    void setMacro(const std::string& name, const Vector4&   val, bool optional = false);
    /** Becomes float in GLSL */
    void setMacro(const std::string& name, const Color1&    val, bool optional = false);
    void setMacro(const std::string& name, const Color3&    val, bool optional = false);
    void setMacro(const std::string& name, const Color4&    val, bool optional = false);
    void setMacro(const std::string& name, const Matrix2&   val, bool optional = false);
    void setMacro(const std::string& name, const Matrix3&   val, bool optional = false);
    void setMacro(const std::string& name, const Matrix4&   val, bool optional = false);

    /** Becomes mat3x4 in GLSL */
    void setMacro(const std::string& name, const CoordinateFrame&   val, bool optional = false);        

    void setUniform(const std::string& name, bool             val, bool optional = false);
    void setUniform(const std::string& name, int              val, bool optional = false);
    void setUniform(const std::string& name, float            val, bool optional = false);
    void setUniform(const std::string& name, uint32           val, bool optional = false);
    void setUniform(const std::string& name, double           val, bool optional = false);
    void setUniform(const std::string& name, const Vector2&   val, bool optional = false);
    void setUniform(const std::string& name, const Vector3&   val, bool optional = false);
    void setUniform(const std::string& name, const Vector4&   val, bool optional = false);

    /** Becomes float in GLSL */
    void setUniform(const std::string& name, const Color1&    val, bool optional = false);
    void setUniform(const std::string& name, const Color3&    val, bool optional = false);
    void setUniform(const std::string& name, const Color4&    val, bool optional = false);
    void setUniform(const std::string& name, const Matrix2&   val, bool optional = false);
    void setUniform(const std::string& name, const Matrix3&   val, bool optional = false);
    void setUniform(const std::string& name, const Matrix4&   val, bool optional = false);

    // Look at Shader to see how this is handled
    void setUniform(const std::string& name, const CoordinateFrame& val, bool optional = false);

    void setUniform(const std::string& name, const Texture::Ref& val, bool optional = false);

}; // class Args

}

#endif
