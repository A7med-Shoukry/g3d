#ifndef G3D_Shader2_h
#define G3D_Shader2_h

#include "G3D/ReferenceCount.h"

namespace G3D {

/**

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

  \section Built-ins

   \subsection gl_FragCoord

   gl_FragCoord.xy is the pixel position of the
   current sample in the output framebuffer.  By default, it refers to
   a pixel center and is half-way between two integer coordinates.
   This can be changed using GLSL directives.

  \section Extensions
  
   \subsection Lines 

   Every source string is automatically prefixed
   with a <code>#line</code> directive assigning it a unique number
   and starting line of 1.  This primarily affects the
   <code>#include</code> directive.  If you use your own
   <code>#line</code> directives, Shader may report incorrect error
   lines to you.
   
   \subsection Preamble
   The entire shader is prefixed with the "G3D Preamble", which defines
   the helper files.  This preamble is considered source string 0.

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
