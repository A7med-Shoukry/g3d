/**
 \file GLG3D/Shader2.h
  
 \maintainer Morgan McGuire, Michael Mara http://graphics.cs.williams.edu
 
 \created 2012-06-13
 \edited 2012-06-13
 */

#ifndef G3D_Shader2_h
#define G3D_Shader2_h

#include "G3D/Matrix2.h"
#include "G3D/Matrix3.h"
#include "G3D/Matrix4.h"
#include "G3D/Vector4.h"
#include "G3D/Vector3.h"
#include "G3D/Vector2.h"
#include "G3D/CoordinateFrame.h"
#include "G3D/Color4.h"
#include "G3D/Color3.h"

#include "G3D/ReferenceCount.h"
#include "GLG3D/VertexRange.h"
#include "GLG3D/Texture.h"
#include "G3D/SmallArray.h"
#include <string>

#include "G3D/constants.h"

namespace G3D {

/**
  \brief Abstraction of the programmable hardware pipeline.  

  TODO List: (* Means tentatively finished)
  Preprocessor
Make all G3D preprocessor extensions recognize spaces between # and the pragma
Make #include insert "#line num filename" automatically before and after the included file so that line numbers and filenames correspond to the original source files
Rewrite "#line num string" as "#line num //G3DFILENAME: string"
Add "#version v1 or v2 or v3 ...", that chooses the latest supported one at parse time and inserts the appropriate pragma
Auto-promote #version to the top line wherever it is encountered
(Keep g3d_sampler2DSize even though OpenGL now provides them; there's no downside and it supports older GL versions)
Auto-define G3D RenderDevice uniforms in a preamble
Define macros for all shader extensions that are supported on the device

Runtime
(Remove old fixed function support for lights and textures)
*Support the host interface for macros listed below
Bind G3D RenderDevice uniforms, if required
Streamline the constructor interface
Support tessellation shaders
Sources from strings or files
Support passing structs and arrays
Shader::reload
Caches compilations with various preambles and macro definitions
When errors are reported, scan through the original source string and read the #line directives to determine the correct filename and line number to report
Make it easy to catch errors on shader compilation and reload
Write warnings to log.txt

GLSL libraries
Add compatibility.glsl
HLSL names
Aliasing of names, e.g., #if __VERSION__ == 120 #define texelFetch texelFetch2D #endif
Scissor/viewport in 8.0 API is underspecified: how do these interact? They should support easy intersection and push/pop.
Draw buffer/read buffer settings should provide useful defaults but allow custom override
Layered-draw buffer interface [1]
Remove old deprecated routines like VertexProgram and VertexAndPixelShader
Must work on recent iMacs
Make G3D assume programmable pipeline and not retain fixed-function state by default



 */ 
 
class Shader2  : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer<Shader2>   Ref;


    enum SourceType {FILE, STRING};

    class Source {
        friend class Shader2;
    protected:
        SourceType type;
        std::string val;
    public:

        Source(SourceType t = STRING, const std::string& value = "");

    };

    /* 
    Shader2::Specification toonShaderSpec;
    toonShaderSpec.vertex = Shader2::Source(Shader2::FILE, "toon.vrt");
    toonShaderSpec.pixel = Shader2::Source(Shader2::FILE, "toon.pix");
    
    m_toonShader = Shader2::create(toonShaderSpec);

    
    
    */
    class Specification {
    public:
        Specification(const Any& any);
        Specification();

        Source vertex;

        /* In DirectX, this is called the Hull Shader */
        Source tessellationControl;

        /* In DirectX, this is called the Domain Shader */
        Source tessellationEvaluation;

        Source geometry;
        Source pixel;
    };

    
    /** All arguments */
    class ArgGroup {

        /* 8-byte storage used for all argument types */
        union Scalar {
            float  f;
            int32  i;
            uint32 ui;
            bool   b;
            double d;
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

        /** Arbitrary string to append to beginning of the shader */
        void setPreamble(const std::string& preamble);
        void setStream(const std::string& name, const VertexRange& val);
        void setStream(GLenum name, const VertexRange& val);

        void setNumInstances(int num);

        // Supports bool, int, float, vec234, mat234
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
        void setMacro(const std::string& name, const Matrix2&   val);
        void setMacro(const std::string& name, const Matrix3&   val);
        void setMacro(const std::string& name, const Matrix4&   val);

        /** Becomes mat3x4 in GLSL */
        void setMacro(const std::string& name, const CoordinateFrame&   val);        

        void setUniform(const std::string& name, bool             val, bool optional = false);
        void setUniform(const std::string& name, int              val);
        void setUniform(const std::string& name, float            val);
        void setUniform(const std::string& name, uint32           val);
        void setUniform(const std::string& name, double           val);
        void setUniform(const std::string& name, const Vector2&   val);
        void setUniform(const std::string& name, const Vector3&   val);
        void setUniform(const std::string& name, const Vector4&   val);

        /** Becomes float in GLSL */
        void setUniform(const std::string& name, const Color1&    val);
        void setUniform(const std::string& name, const Color3&    val);
        void setUniform(const std::string& name, const Color4&    val);
        void setUniform(const std::string& name, const Matrix2&   val);
        void setUniform(const std::string& name, const Matrix3&   val);
        void setUniform(const std::string& name, const Matrix4&   val);

        // Look at Shader to see how this is handled
        void setUniform(const std::string& name, const CoordinateFrame&   val);

        void setUniform(const std::string& name, const Texture::Ref& val);

    }; // class ArgGroup

protected:

    class ProgramObject : public ReferenceCountedObject {
    public:
        typedef ReferenceCountedPointer<ProgramObject> Ref;

        /** Variable declaration discovered in the program */
        class Declaration {
        public:
            GLuint                              location;
            std::string                         name;
            GLenum                              type;
        };

        Table<std::string, Declaration>         m_declarationTable;
    };

    /** Maps preamble + macro definitions to compiled shaders */
    Table<std::string, ProgramObject::Ref>      m_compilationCache;

    Specification                               m_specification;

    bool                                        m_ok;

public:

    ArgGroup args;

    enum FailureBehavior {
        /** Throw an exception on compilation failure */
        EXCEPTION, 
        
        /** Prompt the user to throw an exception, abort the program, or retry loading on compilation failure. (default) */
        PROMPT, 

        /** ok() will be false if an error occurs */
        SILENT
    };


    static FailureBehavior s_failureBehavior;

    bool ok() const;

    static void setFailureBehavior(FailureBehavior f);

    /** Loads and compiles the shader */
    static Ref create(const Specification& s);

    /** Reload this shader from the files on disk, if it was loaded from disk. */
    void reload();

}; // class Shader

}

#endif
