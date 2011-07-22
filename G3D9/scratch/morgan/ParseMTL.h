/**
 \file GLG3D/ParseMTL.h

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2011-07-19
 \edited  2011-07-19

 Copyright 2002-2011, Morgan McGuire.
 All rights reserved.
*/
#ifndef GLG3D_ParseMTL_h
#define GLG3D_ParseMTL_h

//#include <G3D/G3DAll.h>
#include "G3D/platform.h"
#include "G3D/Table.h"
#include "G3D/ReferenceCount.h"
#include "G3D/Color3.h"
#include <string>

namespace G3D {

class TextInput;

/** \brief Parses Wavefront material (.mtl) files.

    Parsing creates references to texture files on disk, but does not actually
    load those textures.

    \sa G3D::ParseOBJ, G3D::ArticulatedModel
*/
class ParseMTL {
public:

    /** Loaded from the MTL file */
    class Material : public ReferenceCountedObject {
    public:
        typedef ReferenceCountedPointer<Material> Ref;
        std::string     name;

        /** Ambient color of the material, on the range 0-1 */
        Color3          Ka;
        std::string     map_Ka;

        /** Diffuse color of the material, on the range 0-1 */
        Color3          Kd;
        std::string     map_Kd;

        /** Specular color of the material, on the range 0-1. */
        Color3          Ks;
        std::string     map_Ks;

        /** Shininess of the material, on the range 0-1000. */
        float           Ns;

        /** map_bump/bump field filename*/
        std::string     map_bump;

        /** Opacity/alpha level, on the range 0-1 */
        float           d;

        /** Transparency level, on the range 0-1. Amount of light transmitted.*/
        float           Tr;

        /** Unknown */
        Color3          Tf;

        /** emissive? */
        Color3          Ke;

        /** Illumination model enumeration on the range 0-10. */
        int             illum;

        /** Index of refraction */
        float           Ni;

        float           bumpBias;
        float           bumpGain;

    private:

        Material() : Ka(1.0f), Kd(1.0f), Ks(1.0f), Ns(10.0), d(1.0f), Tr(0.0f), Tf(1.0f), Ke(0.0f), illum(2), Ni(1.5f) {}

    public:

        static Ref create() {
            return new Material();
        }
    };
    
    Table<std::string, Material::Ref> materialTable;

private:
    /** Process one line of an OBJ file */
    void processCommand(TextInput& ti, const std::string& cmd);

    Material::Ref       m_currentMaterial;

    /** Paths are interpreted relative to this */
    std::string         m_basePath;

public:

    /** \param basePath Directory relative to which texture filenames are resolved. If "<AUTO>", the 
     path to the TextInput%'s file is used. */
    void parse(TextInput& ti, const std::string& basePath = "<AUTO>");

};


template <> struct HashTrait<ParseMTL::Material::Ref> {
    static size_t hashCode(const ParseMTL::Material::Ref& k) { return k.hashCode(); }
};

} // namespace G3D


#endif // #define GLG3D_ParseMTL_h
