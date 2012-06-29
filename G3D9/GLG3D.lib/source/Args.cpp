/**
 \file GLG3D/Args.cpp
  
 \maintainer Morgan McGuire, Michael Mara http://graphics.cs.williams.edu
 
 \created 2012-06-27
 \edited 2012-06-27

 TODO: Add parameter to turn off preprocessor to Specification

 */

#include "G3D/platform.h"
#include "GLG3D/Args.h"
#include "G3D/Matrix2.h"
#include "G3D/CoordinateFrame.h"
#include "G3D/Vector4int16.h"
#include "G3D/Matrix.h"
namespace G3D {

void Args::setPreamble(const std::string& preamble){
    m_preamble = preamble;
}


std::string Args::getPreambleAndMacroString() const{
    std::string preambleAndMacroString = m_preamble + "\n";
    for(int i = 0; i < m_macroArgs.size(); ++i){
        preambleAndMacroString += format("#define %s %s\n", m_macroArgs[i].name, m_macroArgs[i].value.toString());
    }
    return preambleAndMacroString;
}

bool isTextureType(GLenum t){
    return false;
}

/* http://www.opengl.org/wiki/GLSL_Types */
std::string Args::Arg::toString() const {
    switch(type){
    case GL_UNSIGNED_INT:
        return format("%u", value[0].ui);
    case GL_FLOAT:
        return format("%f", value[0].f);
    case GL_FLOAT_VEC2:
        return format("vec2(%f, %f)", value[0].f, value[1].f);
    case GL_FLOAT_VEC3:
        return format("vec3(%f, %f, %f)", value[0].f, value[1].f, value[2].f);
    case GL_FLOAT_VEC4:
        return format("vec4(%f, %f, %f, %f)", value[0].f, value[1].f, value[2].f, value[3].f);
    case GL_INT:
        return format("%d", value[0].i);
    case GL_INT_VEC2:
        return format("ivec2(%d, %d)", value[0].i, value[1].i);
    case GL_INT_VEC3:
        return format("ivec3(%d, %d, %d)", value[0].i, value[1].i, value[2].i);
    case GL_INT_VEC4:
        return format("ivec4(%d, %d, %d %d)", value[0].i, value[1].i, value[2].i, value[3].i);
    case GL_BOOL:
        return format("%s", value[0].b ? "true" : " false");
    case GL_BOOL_VEC2:
        return format("bvec2(%s, %s)", value[0].b ? "true" : " false", value[1].b ? "true" : " false");
    case GL_BOOL_VEC3: 
        return format("bvec3(%s, %s, %s)", value[0].b ? "true" : " false", value[1].b ? "true" : " false",
                        value[2].b ? "true" : " false");
    case GL_BOOL_VEC4: 
        return format("bvec4(%s, %s, %s, %s)", value[0].b ? "true" : " false", value[1].b ? "true" : " false",
                        value[2].b ? "true" : " false", value[3].b ? "true" : " false");
    
    
    // Matrices are column-major in opengl, 
    case GL_FLOAT_MAT2:
        return format("mat2(%f, %f, %f, %f)",
            value[0].f, value[1].f, 
            value[2].f, value[3].f);
    case GL_FLOAT_MAT3:
        return format("mat3(%f, %f, %f, %f, %f, %f, %f, %f, %f)", 
            value[0].f, value[1].f, value[2].f,  
            value[3].f, value[4].f, value[5].f, 
            value[6].f, value[7].f, value[8].f);
    case GL_FLOAT_MAT4:
        return format("mat4(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)", 
            value[0].f,  value[1].f,  value[2].f,  value[3].f,
            value[4].f,  value[5].f,  value[6].f,  value[7].f,
            value[8].f,  value[9].f,  value[10].f, value[11].f,
            value[12].f, value[13].f, value[14].f, value[15].f);
    case GL_FLOAT_MAT2x3:
        return format("mat2x3(%f, %f, %f, %f, %f, %f)", 
            value[0].f, value[1].f, value[2].f,  
            value[3].f, value[4].f, value[5].f);
    case GL_FLOAT_MAT2x4:
        return format("mat2x4(%f, %f, %f, %f, %f, %f, %f, %f)", 
            value[0].f,  value[1].f,  value[2].f,  value[3].f,
            value[4].f,  value[5].f,  value[6].f,  value[7].f);
    case GL_FLOAT_MAT3x2:
        return format("mat3x2(%f, %f, %f, %f, %f, %f)", 
            value[0].f, value[1].f, 
            value[2].f, value[3].f,
            value[4].f, value[5].f);
    case GL_FLOAT_MAT3x4:
        return format("mat3x4(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)", 
            value[0].f,  value[1].f,  value[2].f,  value[3].f,
            value[4].f,  value[5].f,  value[6].f,  value[7].f,
            value[8].f,  value[9].f,  value[10].f, value[11].f);
    case GL_FLOAT_MAT4x2:
        return format("mat4x2(%f, %f, %f, %f, %f, %f, %f, %f)", 
            value[0].f, value[1].f, 
            value[2].f, value[3].f,
            value[4].f, value[5].f,
            value[6].f, value[7].f);
    case GL_FLOAT_MAT4x3:
        return format("mat3(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)", 
            value[0].f, value[1].f,  value[2].f,  
            value[3].f, value[4].f,  value[5].f, 
            value[6].f, value[7].f,  value[8].f,
            value[9].f, value[10].f, value[11].f);


    default:
        // TODO: Figure out what else is not supported
        alwaysAssertM(false, "THIS IS UNSUPPORTED CURRENTLY, SORRY!");
        return "ERROR: Currently unsupported";




    }



}

///// SET MACRO METHODS


void Args::setMacro(const std::string& name, const std::string& value){
    for(int i = 0; i < m_macroArgs.size(); ++i){
        if(m_macroArgs[i].name == name){
            m_macroArgs[i].value = value;
            return;
        }
    }
    m_macroArgs.append(MacroArgPair(name,value));
}

void Args::setMacro(const std::string& name, bool val, bool optional = false){
    std::string value = val ? "true" : "false";
    setMacro(name, value);
}

void Args::setMacro(const std::string& name, int val, bool optional = false){
    std::string value = format("%d", val);
    setMacro(name, value);
}
void Args::setMacro(const std::string& name, uint32 val, bool optional = false){
    std::string value = format("%u", val);
    setMacro(name, value);
}

void Args::setMacro(const std::string& name, double val, bool optional = false){
    std::string value = format("%f", val);
    setMacro(name, value);
}

void Args::setMacro(const std::string& name, float val, bool optional = false){
    std::string value = format("%f", val);
    setMacro(name, value);
}

void Args::setMacro(const std::string& name, const Vector2& vec, bool optional = false){
    std::string value = format("vec2(%f, %f)", vec.x, vec.y);
    setMacro(name, value);
}

void Args::setMacro(const std::string& name, const Vector3& vec, bool optional = false){
    std::string value = format("vec3(%f, %f, %f)", vec.x, vec.y, vec.z);
    setMacro(name, value);
}

void Args::setMacro(const std::string& name, const Vector4& vec, bool optional = false){
    std::string value = format("vec4(%f, %f, %f, %f)", vec.x, vec.y, vec.z, vec.w);
    setMacro(name, value);
}

/** Becomes float in GLSL */
void Args::setMacro(const std::string& name, const Color1& col, bool optional = false){
    std::string value = format("%f", col.value);
    setMacro(name, value);
}

void Args::setMacro(const std::string& name, const Color3& col, bool optional = false){
    std::string value = format("vec3(%f, %f, %f)", col.r, col.g, col.b);
    setMacro(name, value);
}

void Args::setMacro(const std::string& name, const Color4& col, bool optional = false){
    std::string value = format("vec4(%f, %f, %f, %f)", col.r, col.g, col.b, col.a);
    setMacro(name, value);
}

void Args::setMacro(const std::string& name, const Vector2int32& vec, bool optional = false){
    std::string value = format("ivec2(%d, %d)", vec.x, vec.y);
    setMacro(name, value);
}

void Args::setMacro(const std::string& name, const Vector3int32& vec, bool optional = false){
    std::string value = format("ivec3(%d, %d, %d)", vec.x, vec.y, vec.z);
    setMacro(name, value);
}

void Args::setMacro(const std::string& name, const Vector2int16& vec, bool optional = false){
    std::string value = format("ivec2(%d, %d)", vec.x, vec.y);
    setMacro(name, value);
}

void Args::setMacro(const std::string& name, const Vector3int16& vec, bool optional = false){
    std::string value = format("ivec3(%d, %d, %d)", vec.x, vec.y, vec.z);
    setMacro(name, value);
}

void Args::setMacro(const std::string& name, const Vector4int16& vec, bool optional = false){
    std::string value = format("ivec4(%d, %d, %d, %d)", vec.x, vec.y, vec.z, vec.w);
    setMacro(name, value);
}

void Args::setMacro(const std::string& name, const Matrix2& mat, bool optional = false){
    std::string value = format("mat2(%f, %f, %f, %f)", mat[0][0], mat[1][0], mat[0][1], mat[1][1]);
    setMacro(name, value);
}

void Args::setMacro(const std::string& name, const Matrix3& mat, bool optional = false){
    std::string value = format("mat3(%f, %f, %f,", mat[0][0], mat[1][0], mat[2][0]); // Column 0
    value += format(" %f, %f, %f,", mat[0][1], mat[1][1], mat[2][1]); // Column 1
    value += format(" %f, %f, %f)", mat[0][1], mat[1][1], mat[2][1]); // Column 2
    setMacro(name, value);
}
void Args::setMacro(const std::string& name, const Matrix4& mat, bool optional = false){
    std::string value = format("mat3(%d, %d, %d,", mat[0][0], mat[1][0], mat[2][0]); // Column 0
    value += format(" %d, %d, %d,", mat[0][1], mat[1][1], mat[2][1]); // Column 1
    value += format(" %d, %d, %d)", mat[0][1], mat[1][1], mat[2][1]); // Column 2
    setMacro(name, value);
}

/** Becomes mat3x4 in GLSL */
//void Args::setMacro(const std::string& name, const CoordinateFrame&   val, bool optional = false);

//void Args::setMacro(const std::string& name, const Matrix& val, bool optional = false);





void Args::setUniform(const std::string& name, bool val, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.type = GL_BOOL;
    arg.optional = optional;
    arg.value.append(val);
}

void Args::setUniform(const std::string& name, int32 val, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.type = GL_INT;
    arg.optional = optional;
    arg.value.append(val);
}

void Args::setUniform(const std::string& name, uint32 val, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.type = GL_UNSIGNED_INT;
    arg.optional = optional;
    arg.value.append(val);
}


void Args::setUniform(const std::string& name, double val, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.type = GL_DOUBLE;
    arg.optional = optional;
    arg.value.append(val);
}

void Args::setUniform(const std::string& name, float val, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.type = GL_FLOAT;
    arg.optional = optional;
    arg.value.append(val);
}



void Args::setUniform(const std::string& name, const Color1& col, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.type = GL_FLOAT;
    arg.optional = optional;
    arg.value.append(col.value);
}

void Args::setUniform(const std::string& name, const Vector2& vec, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.type = GL_FLOAT_VEC2;
    arg.optional = optional;
    arg.value.append(vec.x);
    arg.value.append(vec.y);
}

void Args::setUniform(const std::string& name, const Vector3& vec, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.type = GL_FLOAT_VEC3;
    arg.optional = optional;
    arg.value.append(vec.x);
    arg.value.append(vec.y);
    arg.value.append(vec.z);
}

void Args::setUniform(const std::string& name, const Vector4& vec, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.type = GL_FLOAT_VEC4;
    arg.optional = optional;
    arg.value.append(vec.x);
    arg.value.append(vec.y);
    arg.value.append(vec.z);
    arg.value.append(vec.w);
}


void Args::setUniform(const std::string& name, const Color3& col, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.type = GL_FLOAT_VEC3;
    arg.optional = optional;
    arg.value.append(col.r);
    arg.value.append(col.g);
    arg.value.append(col.b);
    
}

void Args::setUniform(const std::string& name, const Color4& col, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.type = GL_FLOAT_VEC4;
    arg.optional = optional;
    arg.value.append(col.r);
    arg.value.append(col.g);
    arg.value.append(col.b);
    arg.value.append(col.a);
    
}

void Args::setUniform(const std::string& name, const Matrix2& mat, bool optional){
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.type = GL_FLOAT_MAT2;
    arg.optional = optional;
    for(int i = 0; i < 2; ++i){
        for(int j = 0; j < 2; ++j){
            arg.value.append(mat[j][i]);
        }
    }
    
}

void Args::setUniform(const std::string& name, const Matrix3& mat, bool optional){
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.type = GL_FLOAT_MAT3;
    arg.optional = optional;
    for(int i = 0; i < 3; ++i){
        for(int j = 0; j < 3; ++j){
            arg.value.append(mat[j][i]);
        }
    }
    
}

void Args::setUniform(const std::string& name, const Matrix4& mat, bool optional){
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.type = GL_FLOAT_MAT4;
    arg.optional = optional;
    for(int i = 0; i < 3; ++i){
        for(int j = 0; j < 3; ++j){
            arg.value.append(mat[j][i]);
        }
    }
    
}

void Args::setUniform(const std::string& name, const CoordinateFrame& cframe, bool optional){
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.type = GL_FLOAT_MAT4x3;
    arg.optional = optional;
    for(int i = 0; i < 3; ++i){
        for(int j = 0; j < 3; ++j){
            arg.value.append(cframe.rotation[j][i]);
        }
    }
    for(int j = 0; j < 3; ++j){
        arg.value.append(cframe.translation[j]);
     }
    
}


void Args::setUniform(const std::string& name, const Vector2int32& vec, bool optional){
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.type = GL_INT_VEC2;
    arg.optional = optional;
    arg.value.append(vec.x);
    arg.value.append(vec.y);
    
}
void Args::setUniform(const std::string& name, const Vector3int32&  vec, bool optional){
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.type = GL_INT_VEC3;
    arg.optional = optional;
    arg.value.append(vec.x);
    arg.value.append(vec.y);
    arg.value.append(vec.z);
    
}

void Args::setUniform(const std::string& name, const Vector2int16&  vec, bool optional){
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.type = GL_INT_VEC2;
    arg.optional = optional;
    arg.value.append((int32)vec.x);
    arg.value.append((int32)vec.y);
    
}

void Args::setUniform(const std::string& name, const Vector3int16&  vec, bool optional){
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.type = GL_INT_VEC3;
    arg.optional = optional;
    arg.value.append((int32)vec.x);
    arg.value.append((int32)vec.y);
    arg.value.append((int32)vec.z);
}

void Args::setUniform(const std::string& name, const Vector4int16&  vec, bool optional){
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.type = GL_INT_VEC4;
    arg.optional = optional;
    arg.value.append((int32)vec.x);
    arg.value.append((int32)vec.y);
    arg.value.append((int32)vec.z);
    arg.value.append((int32)vec.w);
}








} // end namespace G3D