/**
 \file GLG3D/Shader2.h
  
 \maintainer Morgan McGuire, Michael Mara http://graphics.cs.williams.edu
 
 \created 2012-06-13
 \edited 2012-06-16
 */

#ifndef GLG3D_Shader2_h
#define GLG3D_Shader2_h

#include "G3D/platform.h"
#include <string>
#include "GLG3D/glheaders.h"
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
#include "GLG3D/Args.h"
#include "G3D/SmallArray.h"

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
*Sources from strings or files
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



    Commonly initialized with the idiom:

    \code
    m_toonShader = Shader2::fromFiles("toon.vrt", "toon.pix");
    \endcode

    or

    \code
    Shader2::Specification toonShaderSpec;
    toonShaderSpec[Shader2::VERTEX] = STR( 
    void main() {
        gl_Position = ...
    });

    m_toonShader = Shader2::create(toonShaderSpec);
    \endcode

 */ 
 
class Shader2  : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer<Shader2>   Ref;
            
    enum ShaderStage {
        VERTEX, 
        
        /** In DirectX, this is called the Hull Shader */
        TESSELLATION_CONTROL, 
        
        /** In DirectX, this is called the Domain Shader */
        TESSELLATION_EVAL, 
        
        GEOMETRY, 
        
        PIXEL,

        STAGE_COUNT
    };

    enum SourceType {FILE, STRING};

    class Source {
        friend class Shader2;
    protected:
        SourceType type;
        std::string val;

    public:

        Source();

        Source(SourceType t, const std::string& value);

        /** Throws an error at runtime if the source string appears to be a filename instead of GLSL code */
        Source(const std::string& sourceString);
    };

    class Specification {
    friend class Shader2;
    protected:
        Source shaderStage[5];

    public:
        Specification(const Any& any);
        Specification();

        /** Take in between 1 and 5 filenames, each uniquely corresponding to one of 
            the 5 stages of the shading pipeline.
            We parse each filename, and based on the extension, load it into the specification at the corresponding stage
            The valid extensions are as follows:
            ------------------------------------------
            Shader Stage            |   Extension(s)
            ------------------------------------------
            Vertex                  |   .vrt or .vtx
            ------------------------------------------
            Tesselation Control     |
            (Hull) Shader           |   .ctl or .hul
            ------------------------------------------
            Tesselation Evaluation  |
            (Domain) Shader         |   .evl or .dom
            ------------------------------------------
            Geometry Shader         |       .geo
            ------------------------------------------
            Pixel (Fragment) Shader |   .pix or .frg     
            ------------------------------------------

            If any of the strings passed in is not the empty string or a filename with this extension, an error will be thrown.    
        */
        Specification(
            const std::string& f0, 
            const std::string& f1 = "", 
            const std::string& f2 = "", 
            const std::string& f3 = "", 
            const std::string& f4 = "");

        Source& operator[](ShaderStage s);
        const Source& operator[](ShaderStage s) const;

    };

    
    

protected:



    class ShaderProgram : public ReferenceCountedObject {
        friend class Shader2;
    
        /** Variable declaration discovered in the program */
        class Declaration {
        public:
            GLuint                              location;
            std::string                         name;
            GLenum                              type;
        };

        GLuint                              m_glShaderObject[STAGE_COUNT];

        Table<std::string, Declaration>     m_declarationTable;
        GLuint                              m_glProgramObject;
        bool                                m_ok;

        std::string                         m_messages;
        ShaderProgram(Shader2::Specification s);
    public:
        typedef ReferenceCountedPointer<ShaderProgram> Ref;

        GLuint glShaderProgramObject(){
            return m_glProgramObject;
        }

        static ShaderProgram::Ref create(Shader2::Specification s);

    };

    /** Maps preamble + macro definitions to compiled shaders */
    Table<std::string, ShaderProgram::Ref>      m_compilationCache;

    Specification                               m_specification;

    bool                                        m_ok;

    ShaderProgram::Ref                          m_shaderProgram;

    Shader2(Specification s);

public:

    Args args;

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

    /** Return the OpenGL object corresponding to the last compiled version of this shader */
    GLuint shaderProgram(){
        return m_shaderProgram->glShaderProgramObject();
    }

    /** Loads and compiles the shader */
    static Ref create(const Specification& s);

    /** \copydoc Shader2::Source::Source() */
    static Ref fromFiles(
        const std::string& f0, 
        const std::string& f1 = "", 
        const std::string& f2 = "", 
        const std::string& f3 = "", 
        const std::string& f4 = "");

    /** Reload this shader from the files on disk, if it was loaded from disk. */
    void reload();


    /** Replaces all \#includes in @a code with the contents of the appropriate files.
        It is called recursively, so included files may have includes themselves.
        This is called automatically by the preprocessor, but is public so as to be
        accessible to code like SuperShader that directly manipulates source strings.
        
        TODO: Automatically add #line file directives to included files

        @param dir The directory from which the parent was loaded.
      */
    static void processIncludes(const std::string& dir, std::string& code);


    /** Execute all steps of our preprocessor engine

        Process Includes
        Process Version Line
        Add G3D #defines
        Add G3D uniform declarations
    */
    static void g3dPreprocessor(const std::string& dir, std::string& code);


    /** Reads the code looking for a #version line (spaces allowed after "#"). If one is found, 
        remove it from @param code and put it in @param versionLine, otherwise set versionLine
        to "#version 120\n". The #version line in the original source CANNOT be the last line
        (we depend on the newline, TODO: remove this dependency)
    
        @return True if we found and removed a version line from the code.
    */
    static bool processVersion(std::string& code, std::string& versionLine);


}; // class Shader

}

#endif
