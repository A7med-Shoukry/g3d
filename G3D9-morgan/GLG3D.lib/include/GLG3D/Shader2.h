#ifndef G3D_Shader2_h
#define G3D_Shader2_h

#include "G3D/ReferenceCount.h"

namespace G3D {

class Shader2 : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer<Shader2> Ref;

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
