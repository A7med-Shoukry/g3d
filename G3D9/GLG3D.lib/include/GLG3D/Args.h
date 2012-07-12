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
#include "G3D/Matrix.h"

namespace G3D {

/** All arguments */
class Args {
public:
    /** 8-byte storage used for all argument types */
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

    /** This contains the value of a uniform or macro argument passed to shader and its type 
        Macro variables can only be a subset of the possible values */
    class Arg {
    public:
        GLenum                  type;

        /** Empty unless this argument is an OpenGL Sampler */
        Texture::Ref            texture;
        SmallArray<Scalar, 4>   value;
        bool                    optional;


        std::string toString() const;
        
        Arg(){}
        Arg(GLenum t) : type(t){}
        Arg(GLenum t, bool o) : type(t), optional(o){}

    };


    class MacroArgPair {
    public:
        std::string name;
        std::string value;
        MacroArgPair(){}
        MacroArgPair(const std::string& n, const std::string& v) : name(n), value(v) {}
    };
   
    typedef Table<std::string, Arg>                 ArgTable;
    typedef Table<std::string, VertexRange>         AttrTable;
private:
    
    std::string         m_preamble;
    Array<MacroArgPair> m_macroArgs;
    ArgTable            m_uniformArgs;
    AttrTable           m_streamArgs;
    int                 m_numInstances;

    /** If empty, sequential indicies will be used */
    VertexRange         m_indexArray;
    
public:
    Args(){
        geometryInput = PrimitiveType::TRIANGLES;
        patchVertices = 3;
    }
    std::string toString() const;
    PrimitiveType geometryInput;
    PrimitiveType geometryOutput;
    GLint patchVertices;

    void setIndexArray(const VertexRange indArray);

    std::string preambleAndMacroString() const;

    bool hasPreambleOrMacros(){
        return m_preamble != "" || m_macroArgs.size() > 0;
    }

    const AttrTable& attributeTable() const{
        return m_streamArgs;
    }

    const VertexRange& getIndices(){
        return m_indexArray;
    }

    const Arg& getUniform(const std::string& name) const;

    /** Arbitrary string to append to beginning of the shader */
    void setPreamble(const std::string& preamble);
    void setStream(const std::string& name, const VertexRange& val);
    void setStream(GLenum name, const VertexRange& val);

    void setNumInstances(int num);


    
    void setMacro(const std::string& name, const std::string& value);


    // Supports bool, int, float, double, vec234, mat234, mat234x234, ivec234,
    void setMacro(const std::string& name, bool             val);
    void setMacro(const std::string& name, int              val);
    void setMacro(const std::string& name, uint32           val);
    void setMacro(const std::string& name, double           val);
    void setMacro(const std::string& name, float            val);

    void setMacro(const std::string& name, const Vector2&   val);
    void setMacro(const std::string& name, const Vector3&   val);
    void setMacro(const std::string& name, const Vector4&   val);
    /** Becomes float in GLSL */
    void setMacro(const std::string& name, const Color1&    val);
    void setMacro(const std::string& name, const Color3&    val);
    void setMacro(const std::string& name, const Color4&    val);

    void setMacro(const std::string& name, const Vector2int32&  val);
    void setMacro(const std::string& name, const Vector3int32&  val);

    void setMacro(const std::string& name, const Vector2int16&  val);
    void setMacro(const std::string& name, const Vector3int16&  val);
    void setMacro(const std::string& name, const Vector4int16&  val);

    void setMacro(const std::string& name, const Matrix2&   val);
    void setMacro(const std::string& name, const Matrix3&   val);
    void setMacro(const std::string& name, const Matrix4&   val);

    /** Becomes mat3x4 in GLSL */
    void setMacro(const std::string& name, const CoordinateFrame&   val);

    void setMacro(const std::string& name, const Matrix&   val);

    // TODO: Support setting nxm arrays    

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

    void setUniform(const std::string& name, const Vector2int32&  val, bool optional = false);
    void setUniform(const std::string& name, const Vector3int32&  val, bool optional = false);

    void setUniform(const std::string& name, const Vector2int16&  val, bool optional = false);
    void setUniform(const std::string& name, const Vector3int16&  val, bool optional = false);
    void setUniform(const std::string& name, const Vector4int16&  val, bool optional = false);



    void setUniform(const std::string& name, const Matrix2&   val, bool optional = false);
    void setUniform(const std::string& name, const Matrix3&   val, bool optional = false);
    void setUniform(const std::string& name, const Matrix4&   val, bool optional = false);

    void setUniform(const std::string& name, const Matrix&   val, bool optional = false);

    // Look at Shader to see how this is handled
    void setUniform(const std::string& name, const CoordinateFrame& val, bool optional = false);

    void setUniform(const std::string& name, const Texture::Ref& val, bool optional = false);

    


}; // class Args

}

#endif
