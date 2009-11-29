#ifndef G3D_Shader2_h
#define G3D_Shader2_h

#include "G3D/ReferenceCount.h"

namespace G3D {

/**
   Set of Vertex, Geometry, and Pixel functions that execute on the GPU/

   This is an abstraction of OpenGL "Programs".

   Example:

   <pre>
   Shader::Specification spec;

   spec.setPrefix("#version 150\n"
                  "#define RADIUS 2\n"
                  "float square(float x) { return x * x; }\n");
   spec.setVertexString    ("void main() { gl_Position = gl_Vertex; }");
   spec.setGeometryFilename("filter.geo");
   spec.setPixelFilename   ("filter.pix");

   Shader::Ref filterShader = Shader::create(spec);
   </pre>

\section Vertex Vertex Shader

  A vertex shader transforms each element of an input stream of vertex
  attributes.  They typically transform vertices from object space to
  screen space by multiplying by g3d_ObjectToScreenMatrix.  Note that
  the vertex shader typically does not perform a perspective division.

  When a geometry shader is used, the vertex shader typically
  transforms only to world or camera space and lets the geometry
  shader perform the projection and frame buffer transformations.

\section Geometry Geometry Shader
  Geometry shaders collect a set of outputs from the vertex shader and produce a new vertex stream
  that is then rasterized.

  Geometry shaders must include layout qualifiers such as:
<pre>
layout(triangles) in;
layout(triangle_strip, max_vertices = 60) out;
</pre>

  declaring the type of the input and output.  By section 4.3.8 (page 37) of the GLSL 1.50.09 specification 
  and the GL_EXT_geometry_shader4 extension (http://www.opengl.org/registry/specs/EXT/geometry_shader4.txt),
  the accepted inputs are:

   - points
   - lines
   - lines_adjacency
   - triangles
   - triangles_adjacency

  Note that per the GLSL specification, geometry shaders do not
  support QUAD or QUAD_STRIP primitives.  TRI_STRIP and TRI_FAN are
  automatrically converted to TRIANGLES and LINE_STRIP is
  automatically converted to lines.  Under the NV_geometry_shader4
  extension, QUAD and QUAD_STRIP are converted to TRIANGLES as well.

  The accepted outputs are:

   - points
   - line_strip
   - triangle_strip

\subsection Fragment Fragment ("Pixel") Shader

Fragment shaders compute the output values that will be composited
into the frame buffer.  They typically perform shading computations
based on lights, shadow maps, and material parameters (BSDFs).

A note on OpenGL terminology: Pixels are the elements of the frame
buffer. Fragments are the pieces of the rasterized geometry that lie
within a pixel's bounds.  Because DirectX has "pixel shaders", the two
terms are often used interchangably, but the distinction can
occasionally be important: one is an array element and the other is a
small piece of geometry.

  \section Built-ins

  \subsection Vertex

 Section 7.1 (page 70) of the GLSL 1.50.09 specification defines:

<pre>
in int gl_VertexID;
in int gl_InstanceID;
out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
};
</pre>

  \subsection Geometry

 Section 7.1 (page 70) of the GLSL 1.50.09 specification defines:

<pre>
in gl_PerVertex {
vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
} gl_in[];
in int gl_PrimitiveIDIn;
out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
};
out int gl_PrimitiveID;
out int gl_Layer;
</pre>

  \subsection Fragment

   Section 7.2 (page 72) of the GLSL 1.50.09 specification defines:

<pre>
in vec4 gl_FragCoord;
in bool gl_FrontFacing;
in float gl_ClipDistance[];
out float gl_FragDepth;
in vec2 gl_PointCoord;
in int gl_PrimitiveID;
</pre>

   \subsection gl_FragCoord

   gl_FragCoord.xy is the pixel position of the current sample in the
   output frame buffer.  By default, it refers to a pixel center and is
   half-way between two integer coordinates and is centered at the
   lower-left of the window.  This can be changed using GLSL
   directives.  

   We recommend using: <code>layout (origin_upper_left)
   in vec4 gl_FragCoord;</code> in your prefix so that gl_FragCoord
   matches the G3D texture coordinates and RenderDevice 2D mode
   coordinates.

   \subsection Matrices
  \htmlonly
  <table>
  <tr><td><code>uniform in mat4x3 g3d_ObjectToWorldMatrix</code></td><td>   </td></tr>

  <tr><td><code>uniform in mat4x3 g3d_WorldToCameraMatrix</code></td><td>   </td></tr>

  <tr><td><code>uniform in mat4   g3d_ProjectionMatrix</code>   </td><td>Projection matrix defined by RenderDevice.</td></tr>

  <tr><td><code>uniform in mat4   g3d_FrameBufferMatrix</code>  </td><td>Used by RenderDevice to invert the Y axis when rendering to a texture so that the upper-left corner of the texture corresponds to texture coordinate (0,0).  The identity matrix when rendering to the screen.</td></tr>

  <tr><td><code>uniform in mat4   g3d_ProjectionFrameBufferMatrix</code></td><td> 
                 <code>= g3d_FrameBufferMatrix * g3d_ProjectionMatrix</code></td></tr>

  <tr><td><code>uniform in mat4   g3d_ObjectToScreenMatrix</code></td><td> 
                 <code>= g3d_FrameBufferMatrix * g3d_ProjectionMatrix * mat4(g3d_WorldToCameraMatrix) * mat4(g3d_ObjectToWorldMatrix)</code>.</td></tr>
  </table>
  \endhtmlonly
       

  \section Pragmas
  
   \subsection Lines 

   Every source string is automatically prefixed
   with a <code>#line</code> directive assigning it a unique number
   and starting line of 1.  This primarily affects the
   <code>#include</code> directive.  If you use your own
   <code>#line</code> directives, Shader may report incorrect error
   lines to you.
   
   \subsection Include

   The custom directive:

   <code>#include "sourcefile"</code>

   Includes the named source file as if its contents had been directly
   inserted at that location.  That is, it concatenates the begining
   of the current string, the contents of sourcefile, a
   <code>#line</code> directive to maintain correct line numbers, and
   the remainder of the current string.
 
  \subsection Version

  Any <code>#version</code> directive encounted in a source string
  is commented out by the G3D preprocessor and that directive is moved
  to the first line of the G3D preamble.

  \section Preamble
   The entire shader is prefixed with the "G3D Preamble", which defines
   the helper files.  This preamble is considered source string 0.

 */
class Shader2 : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer<Shader2> Ref;

    /** Source for one shader (e.g., vertex, geometry, or pixel) */
    class Source {
    public:
        class Section {
        private:

            /**
               "included from <filename>:<line number>\n" repeated for
               each recursive include.  Empty if the root, and 
               "G3D shader preamble" if the G3D preamble.
             */
            std::string   includeHistory;

            /** One of:
                - the source filename
                - "<string>"
                - "<G3D Preamble>"
                - "<prefix>"
               */
            std::string   filename;

            /** Substring from the original code, always ending in a newline. */
            std::string   code;

            /** Number for the file or source string from which \a code is a substring. */
            int           stringNumber;

            /** The first line of \a code corresponds to this line in \a filename. */
            int           firstLineNumber;

            /** The last line of \a code corresponds to this line in \a filename. */
            int           lastLineNumber;
        };

        /** The index is the source string number */
        Array<Section>  sectionArray;

        int             lastStringNumber;

        /** Returns a new unique string number */
        int newStringNumber() {
            ++lastStringNumber;
            return lastStringNumber;
        }

        /** Add a substring*/ 
        void add
        (const std::string& filename,
         int                stringNumber,
         const std::string& contents, 
         const std::string& includeHistory,
         int                startLine) {

            // Look for any #include directives of the form:
            // <line start> <whitespace>* # <whitespace>* include <whitespace>*

            includeLine = find include;
            if (includeLine > -1) {
                separate into begin, include, end;
                
                addSection(filename, stringNumber, begin, includeHistory, startLine);
                
                std::string includeContents = load includeFilename;
                
                // Recursive call to parse the included file
                add(includeFilename,
                    newStringNumber(),
                    includeContents, 
                    "included from " + filename + ":" +
                    format("%d", includeLine + startLine) + NEWLINE + includeHistory,
                    1);

                // Recursive call to parse the rest
                add(filename, stringNumber, end, includeHistory, includeLine + 1);
                
            } else {
                addSection(filename, stringNumber, contents, includeHistory, startLine);
            }
        }


        void addSection
        (const std::string& filename,
         int                stringNumber,
         const std::string& contents, 
         const std::string& includeHistory,
         int                startLine) {
            // TODO
        }

    public:

        Source
        (std::string G3DPreamble,
         std::string prefix,
         std::string filename,
         std::string contents) : lastStringNumber(-1) {
            
            if (beginsWith(contents, "#version ")) {
                // Comment out the first line of contents and
                // move it to the first line of the G3DPreamble

                // TODO: move the first line of contents to the preamble
                
                contents = "// " + contents; 
            }
            
            add("<G3D preamble>", newStringNumber(), G3DPreamble, "", 1);
            add("<prefix>", newStringNumber(), prefix, "", 1);
            add(filename, newStringNumber(), contents, "", 1);
        }

    public:
        
        /** Produces the source code for the entire shader. */
        void getConcatenatedShader(std::string& str) const {
            str = "";
            for (int i = 0; i < sectionArray; ++i) {
                const Section& section = sectionArray[i];
                str += format("#line %d %d\n", section.stringNumber, section.firstLineNumber - 1);
                str += section.code;
            }
            return str;
        }

        /** Given a stringNumber and lineNumber (usually, from an error message),
            returns the original filename, include directives that led to that 
            file, and the contents of the line, ending with a newline.
        */
        void getLine(int stringNumber, 
                     int lineNumber, 
                     std::string& includeString,
                     std::string& filename,
                     std::string& line) const {
            // TODO
        }
    };

    class Specification {
    private:
        
        class CodeSource {
        public:
            bool        fromFile;
            std::string filename;

            /** Code before modification */
            std::string code;
        };

        /** Insert after #version and before the next line. */
        std::string m_prefix;

        CodeSource  m_vertex;
        CodeSource  m_geometry;
        CodeSource  m_pixel;

    public:
    };
    
public:

    static Ref create(const Specification& spec);

    void load(Specification& spec);

    /** Specification used to create this. */
    const Specification& specification() const;

    void reload();

};

}

#endif
