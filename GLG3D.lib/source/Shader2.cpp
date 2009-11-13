/**
 @file Shader2.cpp
 
 */

#include "G3D/fileutils.h"
#include "GLG3D/Shader2.h"


namespace G3D {

UniformVar::UniformVar(const std::string& name) :
    m_type(GL_NONE), m_value(NULL), m_length(0), m_name(name) {
}

UniformVar::~UniformVar() {
    if (m_value) {
        System::alignedFree(m_value);
    }
}

int UniformVar::typeSize() {
    debugAssert(m_type != GL_NONE);
    switch (m_type)
    {
        case GL_TEXTURE_2D:
        case GL_INT:
        case GL_FLOAT:
        {
            return 4;
        }

        case GL_FLOAT_MAT2_ARB:
        {
            return 4 * 4;
        }
        case GL_FLOAT_MAT3_ARB:
        {
            return 4 * 9;
        }
        case GL_FLOAT_MAT4_ARB:
        {
            return 4 * 16;
        }

        case GL_FLOAT_VEC2_ARB:
        {
            return 4 * 2;
        }
        case GL_FLOAT_VEC3_ARB:
        {
            return 4 * 3;
        }
        case GL_FLOAT_VEC4_ARB:
        {
            return 4 * 4;
        }

        default:
        {
            debugAssertM(false, "Unknown type passed to UniformVar.");
            return 0;
        }
    }
}

void UniformVar::setupValue(GLenum type, int length) {
    debugAssert(m_type == GL_NONE || m_type == type);
    debugAssert(m_length == 0 || m_length == length);

    m_type = type;
    m_length = length;

    // this assumes the asserts above passed
    if (!m_value) {
        // allocate value buffer if it doesn't already exist (re-assigning to a value)
        m_value = System::alignedMalloc(typeSize() * m_length, 16 * iCeil(typeSize() / 16.f));
    }
}

UniformVar& UniformVar::operator=(const Texture::Ref& value) {
    // the actual texture type doesn't matter when binding
    setupValue(GL_TEXTURE_2D, 1);

    int* iVal = static_cast<int*>(m_value);
    iVal[0] = value->openGLTextureTarget();

    return (*this);
}

UniformVar& UniformVar::operator=(const Array<Texture::Ref>& value) {
    // the actual texture type doesn't matter when binding
    setupValue(GL_TEXTURE_2D, value.length());

    int* iVal = static_cast<int*>(m_value);
    for (int texIndex = 0; texIndex < value.length(); ++texIndex) {
        iVal[texIndex] = value[texIndex]->openGLTextureTarget();
    }

    return (*this);
}

UniformVar& UniformVar::operator=(float value) {
    setupValue(GL_FLOAT, 1);

    int* fVal = static_cast<int*>(m_value);
    fVal[0] = value;

    return (*this);
}

UniformVar& UniformVar::operator=(int value) {
    setupValue(GL_INT, 1);

    int* intVal = static_cast<int*>(m_value);
    intVal[0] = value;

    return (*this);
}

UniformVar& UniformVar::operator=(const Array<float>& value) {
    setupValue(GL_FLOAT, value.length());

    float* fVal = static_cast<float*>(m_value);
    System::memcpy(fVal, &value, sizeof(float) * m_length);

    return (*this);
}

UniformVar& UniformVar::operator=(const Array<int>& value) {
    setupValue(GL_INT, value.length());

    int* iVal = static_cast<int*>(m_value);
    System::memcpy(iVal, &value, sizeof(int) * m_length);

    return (*this);
}

UniformVar& UniformVar::operator=(const Matrix2& value) {
    setupValue(GL_FLOAT_MAT2_ARB, 1);

    float* fVal = static_cast<float*>(m_value);
    System::memcpy(fVal, &value, sizeof(float) * 4);

    return (*this);
}

UniformVar& UniformVar::operator=(const Matrix3& value){
    setupValue(GL_FLOAT_MAT3_ARB, 1);

    int* fVal = static_cast<int*>(m_value);
    System::memcpy(fVal, &value, sizeof(float) * 9);

    return (*this);
}

UniformVar& UniformVar::operator=(const Matrix4& value) {
    setupValue(GL_FLOAT_MAT4_ARB, 1);

    float* fVal = static_cast<float*>(m_value);
    System::memcpy(fVal, &value, sizeof(float) * 16);

    return (*this);
}

UniformVar& UniformVar::operator=(const Array<Matrix2>& value) {
    setupValue(GL_FLOAT_MAT2_ARB, value.length());

    float* fVal = static_cast<float*>(m_value);
    System::memcpy(fVal, &value[0], sizeof(float) * 4 * m_length);

    return (*this);
}

UniformVar& UniformVar::operator=(const Array<Matrix3>& value) {
    setupValue(GL_FLOAT_MAT3_ARB, value.length());

    float* fVal = static_cast<float*>(m_value);
    System::memcpy(fVal, &value[0], sizeof(float) * 9 * m_length);

    return (*this);
}

UniformVar& UniformVar::operator=(const Array<Matrix4>& value) {
    setupValue(GL_FLOAT_MAT4_ARB, value.length());

    float* fVal = static_cast<float*>(m_value);
    System::memcpy(fVal, &value[0], sizeof(float) * 16 * m_length);

    return (*this);
}

UniformVar& UniformVar::operator=(const Vector2& value) {
    setupValue(GL_FLOAT_VEC2_ARB, 1);

    float* fVal = static_cast<float*>(m_value);
    System::memcpy(fVal, &value, sizeof(float) * 2);

    return (*this);
}

UniformVar& UniformVar::operator=(const Vector3& value) {
    setupValue(GL_FLOAT_VEC3_ARB, 1);

    float* fVal = static_cast<float*>(m_value);
    System::memcpy(fVal, &value, sizeof(float) * 3);

    return (*this);
}

UniformVar& UniformVar::operator=(const Vector4& value) {
    setupValue(GL_FLOAT_VEC4_ARB, 1);

    float* fVal = static_cast<float*>(m_value);
    System::memcpy(fVal, &value, sizeof(float) * 3);

    return (*this);
}

UniformVar& UniformVar::operator=(const Array<Vector2>& value) {
    setupValue(GL_FLOAT_VEC2_ARB, value.length());

    float* fVal = static_cast<float*>(m_value);
    System::memcpy(fVal, &value[0], sizeof(float) * 2 * m_length);

    return (*this);
}

UniformVar& UniformVar::operator=(const Array<Vector3>& value) {
    setupValue(GL_FLOAT_VEC3_ARB, value.length());

    float* fVal = static_cast<float*>(m_value);
    System::memcpy(fVal, &value[0], sizeof(float) * 3 * m_length);

    return (*this);
}

UniformVar& UniformVar::operator=(const Array<Vector4>& value) {
    setupValue(GL_FLOAT_VEC4_ARB, value.length());

    float* fVal = static_cast<float*>(m_value);
    System::memcpy(fVal, &value[0], sizeof(float) * 4 * m_length);

    return (*this);
}

///////////////////////////////////

Shader2::Ref Shader2::fromFiles(const std::string& baseFilename, const Array<std::string>& macros) {

    // Load vertex and pixel shaders then geometry shader if available
    Shader2* shader = new Shader2;

    bool success = false;

    success = shader->loadUnits(baseFilename);

    if (success) {
        success = shader->linkUnits();
    }

    if (! success) {
        delete shader;
        shader = NULL;
    }

    return shader;
}


bool Shader2::loadUnits(const std::string& baseFilename) {
    // Load vertex and pixel shaders then geometry shader if available
    std::string vrtFilename = baseFilename + ".vrt";

    if (! fileExists(vrtFilename)) {
        debugAssertM(false, "Must define vertex shader!");
        return false;
    }

    std::string pixFilename = baseFilename + ".pix";

    if (! fileExists(pixFilename)) {
        debugAssertM(false, "Must define pixel shader!");
        return false;
    }

    // Create vertex unit
    Unit* vrtUnit = new Unit(TYPE_VERTEX);
    m_units.append(vrtUnit);

    // Load vertex shader
    if (! vrtUnit->load(m_programObject, vrtFilename)) {
        debugAssertM(false, "Unable to load vertex shader!");
        return false;
    }

    // Create pixel unit
    Unit* pixUnit = new Unit(TYPE_PIXEL);
    m_units.append(pixUnit);

    // Load pixel shader
    if (! pixUnit->load(m_programObject, pixFilename)) {
        debugAssertM(false, "Unable to load pixel shader!");
        return false;
    }

    std::string geoFilename = baseFilename + ".geo";
    Unit* geoUnit = NULL;

    // Load geometry shader
    if (fileExists(geoFilename)) {
        geoUnit = new Unit(TYPE_GEOMETRY);
        m_units.append(geoUnit);

        if (! geoUnit->load(m_programObject, geoFilename)) {
            debugAssertM(false, "Unable to load geometry shader!");
            return false;
        }
    }

    return true;
}

bool Shader2::linkUnits() {
    // loop through all units and link each type
    return true;
}

bool Shader2::Unit::load(GLhandleARB programObject, const std::string& code) {
    bool success = false;

    // create mutable copy of code
    std::string processedCode = code;

    success = preprocess(processedCode);

    if (success) {
        success = compile(processedCode);
    }

    if (success) {
        success = attach(programObject);
    }

    return success;
}

bool Shader2::Unit::preprocess(std::string& code) {
    // process includes

    // process g3d uniforms

    // process macros
    return true;
}

bool Shader2::Unit::compile(const std::string& code) {
    // compile
    return true;
}

bool Shader2::Unit::attach(GLhandleARB programObject) {
    // Attach unit
    glAttachObjectARB(programObject, m_ShaderObject);
    return true;
}


} // namespace G3D
