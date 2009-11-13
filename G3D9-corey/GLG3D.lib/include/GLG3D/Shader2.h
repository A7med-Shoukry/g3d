/**
 @file Shader2.h
  
 */
#ifndef G3D_SHADER2_H
#define G3D_SHADER2_H

#include <string>
#include "G3D/Array.h"
#include "G3D/Matrix2.h"
#include "G3D/Matrix4.h"
#include "G3D/Vector2.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/Texture.h"


namespace G3D {

/**
    Contains information about a single, bind-able uniform
    variable in a linked shader.
 */
class UniformLocation {
private:
    std::string m_name;
    int         m_location;

public:
    UniformLocation() : m_location(0) {}
    
    void SetLocation(const std::string& name, int location);
};

/**
    Contains a single user-defined uniform variable
 */
class UniformVar {
private:
    GLenum      m_type;
    void*       m_value;
    int         m_length;
    std::string m_name;

    int     typeSize();
    void    setupValue(GLenum type, int length);

public:
    UniformVar(const std::string& name);
    virtual ~UniformVar();

    GLenum      type()      { return m_type; }
    void*       value()     { return m_value; }
    int         length()    { return m_length; }
    std::string name()      { return m_name; }

    UniformVar& operator=(const Texture::Ref& value);

    UniformVar& operator=(const Array<Texture::Ref>& value);

    UniformVar& operator=(float value);
    UniformVar& operator=(int value);

    UniformVar& operator=(const Array<float>& value);
    UniformVar& operator=(const Array<int>& value);

    UniformVar& operator=(const Matrix2& value);
    UniformVar& operator=(const Matrix3& value);
    UniformVar& operator=(const Matrix4& value);

    UniformVar& operator=(const Array<Matrix2>& value);
    UniformVar& operator=(const Array<Matrix3>& value);
    UniformVar& operator=(const Array<Matrix4>& value);

    UniformVar& operator=(const Vector2& value);
    UniformVar& operator=(const Vector3& value);
    UniformVar& operator=(const Vector4& value);

    UniformVar& operator=(const Array<Vector2>& value);
    UniformVar& operator=(const Array<Vector3>& value);
    UniformVar& operator=(const Array<Vector4>& value);
};

/**
    Contains all uniform variables that are bound to each
    vertex, geometry and pixel shader loaded.
 */
class UniformArgs {
private:

public:
    UniformArgs();
    virtual ~UniformArgs();

    UniformVar& operator[](const std::string& name);

    void bindLocations(const Array<UniformLocation>& locations);
};



class Shader2 : public ReferenceCountedObject {
private:
    enum UnitType {
        TYPE_VERTEX,
        TYPE_PIXEL,
        TYPE_GEOMETRY
    };

    /** Represents the individual vertex, geometry, and pixel shaders. */
    class Unit {
    private:
        GLhandleARB m_ShaderObject;
        UnitType    m_type;

        bool        preprocess(std::string& code);
        bool        compile(const std::string& code);
        bool        attach(GLhandleARB programObject);
        
    public:
        Unit(UnitType type) : m_ShaderObject(0), m_type(type) {}
        ~Unit();

        UnitType    type() const    { return m_type; }

        bool        load(GLhandleARB programObject, const std::string& code);
    };

    GLhandleARB             m_programObject;
    Array<Unit*>            m_units;
    Array<UniformLocation>  m_locations;

    bool loadUnits(const std::string& baseFilename);
    bool linkUnits();

    Shader2() : m_programObject(0) {}

public:
    typedef ReferenceCountedPointer<class Shader2>  Ref;

    /**
        Loads \a baseFilename + ".vrt", ".geo", and ".pix".
        Default shader is used for any missing files unless
        all files are missing.  All files missing should throw
        an exception (todo).
     */
    static Shader2::Ref     fromFiles(const std::string& baseFilename,
                                      const Array<std::string>& macros = Array<std::string>());


    void bind(const UniformArgs& args);
    
};


} // namespace G3D

#endif // G3D_SHADER2_H
