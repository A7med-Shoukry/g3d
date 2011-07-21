#ifndef GLG3D_ParseMTL_h
#define GLG3D_ParseMTL_h

#include <G3D/G3DAll.h>

/** \brief Parses Wavefront material (.mtl) files.

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

#endif
