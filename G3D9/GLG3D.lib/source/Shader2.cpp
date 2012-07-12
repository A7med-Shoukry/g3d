/**
 \file GLG3D/Shader2.cpp
  
 \maintainer Morgan McGuire, Michael Mara http://graphics.cs.williams.edu
 
 \created 2012-06-13
 \edited 2012-06-13

 TODO: Add parameter to turn off preprocessor to Specification



 */

#include "G3D/platform.h"
#include "GLG3D/Shader2.h"
#include "GLG3D/GLCaps.h"
#include "G3D/fileutils.h"
#include "G3D/Log.h"
#include "G3D/FileSystem.h"
#include "G3D/prompt.h"
#include "GLG3D/RenderDevice.h"
namespace G3D {

Shader2::FailureBehavior Shader2::s_failureBehavior = Shader2::PROMPT;



/** Converts a type name to a GL enum */
static GLenum toGLType(const std::string& s) {
    if (s == "float") {
        return GL_FLOAT;
    } else if (s == "vec2") {
        return GL_FLOAT_VEC2;
    } else if (s == "vec3") {
        return GL_FLOAT_VEC3;
    } else if (s == "vec4") {
        return GL_FLOAT_VEC4;

    } else if (s == "ivec2") {
        return GL_INT_VEC2;

    } else if (s == "int") {
        return GL_INT;
    } else if (s == "unsigned int") {
        return GL_UNSIGNED_INT;

    } else if (s == "bool") {
        return GL_BOOL;

    } else if (s == "mat2") {
        return GL_FLOAT_MAT2;
    } else if (s == "mat3") {
        return GL_FLOAT_MAT3;
    } else if (s == "mat4") {
        return GL_FLOAT_MAT4;
    } else if (s == "mat4x3") {
        return GL_FLOAT_MAT4x3;

    } else if (s == "sampler1D") {
        return GL_SAMPLER_1D;
    } else if (s == "isampler1D") {
        return GL_INT_SAMPLER_1D;
    } else if (s == "usampler1D") {
        return GL_UNSIGNED_INT_SAMPLER_1D;

    } else if (s == "sampler2D") {
        return GL_SAMPLER_2D;
    } else if (s == "isampler2D") {
        return GL_INT_SAMPLER_2D;
    } else if (s == "usampler2D") {
        return GL_UNSIGNED_INT_SAMPLER_2D;

    } else if (s == "sampler3D") {
        return GL_SAMPLER_3D;
    } else if (s == "isampler3D") {
        return GL_INT_SAMPLER_3D;
    } else if (s == "usampler3D") {
        return GL_UNSIGNED_INT_SAMPLER_3D;

    } else if (s == "samplerCube") {
        return GL_SAMPLER_CUBE;
    } else if (s == "isamplerCube") {
        return GL_INT_SAMPLER_CUBE;
    } else if (s == "usamplerCube") {
        return GL_UNSIGNED_INT_SAMPLER_CUBE;

    } else if (s == "sampler2DRect") {
        return GL_SAMPLER_2D_RECT;
    } else if (s == "usampler2DRect") {
        return GL_UNSIGNED_INT_SAMPLER_2D_RECT;
    } else if (s == "sampler2DShadow") {
        return GL_SAMPLER_2D_SHADOW;
    } else if (s == "sampler2DRectShadow") {
        return GL_SAMPLER_2D_RECT_SHADOW;
    } else {
        debugAssertM(false, std::string("Unknown type in shader: ") + s);
        return 0;
    }
}




Shader2::Shader2(Specification s){
    m_specification = s;
}

bool Shader2::isSamplerType(GLenum e) {
    // TODO: Update to remove "ARB"
    return
       (e == GL_SAMPLER_1D_ARB) ||
       (e == GL_UNSIGNED_INT_SAMPLER_1D) ||

       (e == GL_SAMPLER_2D_ARB) ||
       (e == GL_INT_SAMPLER_2D) ||
       (e == GL_UNSIGNED_INT_SAMPLER_2D) ||
       (e == GL_SAMPLER_2D_RECT_ARB) ||

       (e == GL_SAMPLER_3D_ARB) ||
       (e == GL_INT_SAMPLER_3D) ||
       (e == GL_UNSIGNED_INT_SAMPLER_3D) ||

       (e == GL_SAMPLER_CUBE_ARB) ||
       (e == GL_INT_SAMPLER_CUBE) ||
       (e == GL_UNSIGNED_INT_SAMPLER_CUBE) ||

       (e == GL_SAMPLER_1D_SHADOW_ARB) ||

       (e == GL_SAMPLER_2D_SHADOW_ARB) ||
       (e == GL_SAMPLER_2D_RECT_SHADOW_ARB);
}


void Shader2::ShaderProgram::addUniformsFromSource(const Array<PreprocessedShaderSource>& preprocessedSource){
    for(int s = 0; s < STAGE_COUNT; ++s){
        addUniformsFromCode(preprocessedSource[s].preprocessedCode);
    }
}


void Shader2::ShaderProgram::addUniformsFromCode(const std::string& code) {
    TextInput ti(TextInput::FROM_STRING, code);
    while (ti.hasMore()) {
        if ((ti.peek().type() == Token::SYMBOL) && (ti.peek().string() == "uniform")) {
            // Read the definition
            ti.readSymbol("uniform");

            // Maybe read "const"
            if ((ti.peek().type() == Token::SYMBOL) && (ti.peek().string() == "const")) {
                ti.readSymbol("const");
            }

            // Read the type
            std::string variableSymbol = ti.readSymbol();

            // check for "unsigned int"
            if ((variableSymbol == "unsigned") && (ti.peek().string() == "int")) {
                variableSymbol += " " + ti.readSymbol();
            }

            GLenum type = toGLType(variableSymbol);

            // Read the name
            std::string name = ti.readSymbol();
/*
            if ((ti.peek().type() == Token::SYMBOL) && (ti.peek().string() == "[")) {
                ti.readSymbol("[");
                ti.readNumber();
                ti.readSymbol("]");
            }*/

            // Read until the semi-colon
            while (ti.read().string() != ";");

            bool created;

            Declaration& d = declarationTable.getCreate(name, created);
            // See if this variable is already declared.
            if (created) {
                
                d.dummy = true;
                d.location = -1;
                d.name = name;
                d.size = 1;
                d.type = type;

                // Don't allocate texture units for unused variables
                d.textureUnit = -1;
            }

        } else {
            // Consume the token
            ti.read();
        }
    }
}

/*
void Shader2::ShaderProgram::addVertexAttributesFromSource(const Array<PreprocessedShaderSource>& preprocessedSource) {
    
    const std::string& code = preprocessedSource[VERTEX].preprocessedCode;
    TextInput ti(TextInput::FROM_STRING, code);
    while (ti.hasMore()) {
        if ((ti.peek().type() == Token::SYMBOL) && (ti.peek().string() == "in")) {
            // Read the definition
            ti.readSymbol("in");

            // Read the type
            std::string variableSymbol = ti.readSymbol();

            // check for "unsigned int"
            if ((variableSymbol == "unsigned") && (ti.peek().string() == "int")) {
                variableSymbol += " " + ti.readSymbol();
            }

            GLenum type = toGLType(variableSymbol);

            // Read the name
            std::string name = ti.readSymbol();

            if ((ti.peek().type() == Token::SYMBOL) && (ti.peek().string() == "[")) {
                ti.readSymbol("[");
                ti.readNumber();
                ti.readSymbol("]");
            }

            // Read until the semi-colon
            while (ti.read().string() != ";");

            bool created;

            Declaration& d = declarationTable.getCreate(name, created);
            // See if this variable is already declared.
            if (created) {
                
                d.dummy = true;
                d.location = -1;
                d.name = name;
                d.size = 1;
                d.type = type;

                // Don't allocate texture units for unused variables
                d.textureUnit = -1;
            }

        } else if((ti.peek().type() == Token::SYMBOL) && (ti.peek().string() == "layout")){

            
            ti.readSymbol("layout");

            ti.readSymbol("(");
            ti.readSymbol("location");
            ti.readSymbol("=");
            int attributeIndex = ti.readNumber();
            ti.readSymbol(")");

            // Read the definition
            ti.readSymbol("in");

            // Read the type
            std::string variableSymbol = ti.readSymbol();

            // check for "unsigned int"
            if ((variableSymbol == "unsigned") && (ti.peek().string() == "int")) {
                variableSymbol += " " + ti.readSymbol();
            }

            GLenum type = toGLType(variableSymbol);

            // Read the name
            std::string name = ti.readSymbol();
/*
            if ((ti.peek().type() == Token::SYMBOL) && (ti.peek().string() == "[")) {
                ti.readSymbol("[");
                ti.readNumber();
                ti.readSymbol("]");
            }


            while (ti.read().string() != ";");

            bool created;

            Declaration& d = declarationTable.getCreate(name, created);
            // See if this variable is already declared.
            if (created) {
                
                d.dummy = true;
                d.location = -1;
                d.name = name;
                d.size = 1;
                d.type = type;

                // Don't allocate texture units for unused variables
                d.textureUnit = -1;
            }

        } else {
            // Consume the token
            ti.read();
        }
    }
}
*/


void Shader2::ShaderProgram::computeVertexAttributeTable() {
    // Length of the longest variable name
    GLint maxLength;

    // Number of uniform variables
    GLint attributeCount;

    // Get the number of uniforms, and the length of the longest name.
    glGetProgramiv(glShaderProgramObject(), GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLength);
    glGetProgramiv(glShaderProgramObject(), GL_ACTIVE_ATTRIBUTES, &attributeCount);

    GLchar* name = (GLchar *) System::malloc(maxLength * sizeof(GLchar));
    
    // Get the sizes, types and names

    // Loop over glGetActiveAttribute and store the results away.
    for (GLuint i = 0; i < attributeCount; ++i) {

        
        AttributeDeclaration d;
        glGetActiveAttrib(glShaderProgramObject(), i, maxLength, NULL, &d.size, &d.type, name); 
        d.location = glGetAttribLocation(glShaderProgramObject(), name);
        d.name = name;
        debugAssert(!attributeTable.containsKey(name));
        attributeTable.set(name, d);

    }
    System::free(name);
    name = NULL;
}



void Shader2::ShaderProgram::computeUniformTable() {
    // Length of the longest variable name
    GLint maxLength;

    // Number of uniform variables
    GLint uniformCount;

    // Get the number of uniforms, and the length of the longest name.
    glGetProgramiv(glShaderProgramObject(), GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);
    glGetProgramiv(glShaderProgramObject(), GL_ACTIVE_UNIFORMS, &uniformCount);

    GLchar* name = (GLchar *) malloc(maxLength * sizeof(GLchar));
    
    // Get the sizes, types and names

    int lastTextureUnit = -1;
    // Loop over glGetActiveUniform and store the results away.
    for (int i = 0; i < uniformCount; ++i) {


        const GLuint uniformIndex = i;
        glGetActiveUniformName(glShaderProgramObject(), uniformIndex, maxLength, NULL, name);
        Declaration& d = declarationTable.getCreate(name);

        // TODO: Probably remove. Redundant
        d.name = name;

        // Get location type and size
        d.location = glGetUniformLocation(glShaderProgramObject(), name);
        glGetActiveUniformsiv(glShaderProgramObject(), 1, &uniformIndex, GL_UNIFORM_TYPE, (GLint*) &d.type);
        glGetActiveUniformsiv(glShaderProgramObject(), 1, &uniformIndex, GL_UNIFORM_SIZE, (GLint*) &d.size);
        
        bool isGLBuiltIn = (d.location == -1) || 
            ((strlen(name) > 3) && beginsWith(std::string(name), "gl_"));

        d.dummy = isGLBuiltIn;

        if (! isGLBuiltIn) {
            if (isSamplerType(d.type)) {
                ++lastTextureUnit;
                d.textureUnit = lastTextureUnit;
            } else {
                d.textureUnit = -1;
            }
        }
    }

    free(name);
}


void Shader2::bindUniformArg(const Args::Arg& arg, const ShaderProgram::Declaration& decl){
    debugAssertGLOk();
    GLint location = decl.location;
    // Bind based on the *declared* type
    switch (decl.type) {
    /*case GL_TEXTURE_1D:
        debugAssertM(false, "1D texture binding not implemented");
        break;
        */
    case GL_SAMPLER_1D:
    case GL_SAMPLER_2D:
    case GL_SAMPLER_3D:
    case GL_SAMPLER_CUBE:
    case GL_SAMPLER_1D_SHADOW:
    case GL_SAMPLER_2D_SHADOW:
        // Textures are bound as if they were integers.  The
        // value of the texture is the texture unit into which
        // the texture is placed.
        debugAssert(decl.textureUnit >= 0);
        glUniform1i(decl.location, decl.textureUnit);
       
        // Directly make the OpenGL binding call
        glActiveTexture(decl.textureUnit + GL_TEXTURE0);
        glBindTexture(arg.texture->openGLTextureTarget(), arg.texture->openGLID());
        
        break;

    case GL_INT:
        {
            int32 i = arg.value[0].i;
            glUniform1i(location, i);
        }
        break;

    case GL_BOOL:
        {
            bool b = arg.value[0].b;
            glUniform1i(location, b);
        }
        break;

    case GL_UNSIGNED_INT:
        {
            uint32 ui = arg.value[0].ui;
            glUniform1ui(location, ui);
        }
        break;

    case GL_FLOAT:
        {
            float f = arg.value[0].f;
            glUniform1f(location, f);
        }
        break;

    case GL_FLOAT_VEC2:
        {
            float f0 = arg.value[0].f;
            float f1 = arg.value[1].f;
            glUniform2f(location, f0, f1);
        }
        break;

    case GL_FLOAT_VEC3:
        {
            float f0 = arg.value[0].f;
            float f1 = arg.value[1].f;
            float f2 = arg.value[2].f;
            glUniform3f(location, f0, f1, f2);
        }
        break;

    case GL_FLOAT_VEC4:
        {
            float f0 = arg.value[0].f;
            float f1 = arg.value[1].f;
            float f2 = arg.value[2].f;
            float f3 = arg.value[3].f;
            glUniform4f(location, f0, f1, f2, f3);
        }
        break;

    case GL_INT_VEC2:
        {
            int32 i0 = arg.value[0].i;
            int32 i1 = arg.value[1].i;
            glUniform2i(location, i0, i1);
        }
        break;

    case GL_BOOL_VEC2:
        {
            bool b0 = arg.value[0].b;
            bool b1 = arg.value[1].b;
            glUniform2i(location, b0, b1);
        }
        break;

    case GL_INT_VEC3:
        {
            int32 i0 = arg.value[0].i;
            int32 i1 = arg.value[1].i;
            int32 i2 = arg.value[2].i;
            glUniform3i(location, i0, i1, i2);
        }
        break;

    case GL_BOOL_VEC3:
        {
            bool b0 = arg.value[0].b;
            bool b1 = arg.value[1].b;
            bool b2 = arg.value[2].b;
            glUniform3i(location, b0, b1, b2);
        }
        break;

    case GL_INT_VEC4:
        {
            int32 i0 = arg.value[0].i;
            int32 i1 = arg.value[1].i;
            int32 i2 = arg.value[2].i;
            int32 i3 = arg.value[3].i;
            glUniform4i(location, i0, i1, i2, i3);
        }
        break;

    case GL_BOOL_VEC4:
        {
            bool b0 = arg.value[0].b;
            bool b1 = arg.value[1].b;
            bool b2 = arg.value[2].b;
            bool b3 = arg.value[3].b;
            glUniform4i(location, b0, b1, b2, b3);
        }
        break;

    case GL_FLOAT_MAT2:
        {
            float m[4];
            for (int i = 0; i < 4; ++i) {
                m[i] = arg.value[i].f;
            }
            glUniformMatrix2fv(location, 1, GL_FALSE, m);
        }            
        break;

    case GL_FLOAT_MAT3_ARB:
        {
            float m[9];
            for (int i = 0; i < 9; ++i) {
                m[i] = arg.value[i].f;
            }
            glUniformMatrix3fv(location, 1, GL_FALSE, m);
        }            
        break;

    case GL_FLOAT_MAT4:
        {
            float m[16];
            for (int i = 0; i < 16; ++i) {
                m[i] = arg.value[i].f;
            }
            glUniformMatrix4fv(location, 1, GL_FALSE, m);
        }
        break;

    case GL_FLOAT_MAT4x3:
        {
            float m[12];
            for (int i = 0; i < 12; ++i) {
                m[i] = arg.value[i].f;
            }
            glUniformMatrix4x3fv(location, 1, GL_FALSE, m);
        }
        break;

    default:
        alwaysAssertM(false, format("Unsupported argument type: %s", GLenumToString(decl.type)));
    } // switch on type
    debugAssertGLOk();
}

void Shader2::bindUniformArgs(const Args& args){


    // Iterate through the formal parameter list
    const ShaderProgram::DeclTable& t = m_shaderProgram->declarationTable;
    for (ShaderProgram::DeclTable::Iterator i = t.begin(); i != t.end(); ++i){

        const ShaderProgram::Declaration& decl  = (*i).value;
    
        if (decl.dummy) {
            // Don't set this variable; it is unused.
            continue;
        }

        int location = decl.location;

        if (beginsWith(decl.name, "g3d_sz2D_")) {
            // This is the autogenerated dimensions of a texture.

            std::string textureName  = decl.name.substr(9, decl.name.length() - 9);

            const Args::Arg& arg = args.getUniform(textureName); 

            // Compute the vector of size and inverse size
            float w = arg.texture->width();
            float h = arg.texture->height();
            Vector4 v(w, h, 1.0f / w, 1.0f / h);

            glUniform4fv(location, 1, reinterpret_cast<const float*>(&v));

        } else if (!beginsWith(decl.name, "_noset_")) {

            // Normal user defined variable

            const Args::Arg& arg = args.getUniform(decl.name);
            bindUniformArg(arg, decl);

            
        } // if g3d_sz2D_ variable
    }
}


void Shader2::bindStreamArgs(const Args& args, RenderDevice* rd){

    static Array<std::string> g3dAttributes;
    static bool init = true;
    if(init){
        init = false;
        g3dAttributes.append("g3d_Vertex", "g3d_Normal", "g3d_Color", "g3d_MultiTexCoord0");
        g3dAttributes.append("g3d_MultiTexCoord1", "g3d_MultiTexCoord2", "g3d_MultiTexCoord3");
        g3dAttributes.append("g3d_MultiTexCoord4", "g3d_MultiTexCoord5", "g3d_MultiTexCoord6",
                            "g3d_MultiTexCoord7");
    }

    // Iterate through the formal parameter list
    const Args::AttrTable&  t                                   = args.attributeTable();
    const ShaderProgram::AttrTable& attributeInformationTable   = m_shaderProgram->attributeTable;
    bool usedGLBuiltin = false;
    bool usedGeneric   = false;
    for (Args::AttrTable::Iterator i = t.begin(); i != t.end(); ++i){

        const VertexRange& v    = (*i).value;
        const std::string& name = (*i).key;

        if (beginsWith(name, "gl_")) { // Built-in, deprecated-style assignment
            usedGLBuiltin = true;
            if(name == "gl_Vertex"){
                rd->setVertexArray(v);
            } else if(name == "gl_Normal") {
                rd->setNormalArray(v);
            } else if(name == "gl_Color") {
                rd->setColorArray(v);
            } else if(beginsWith(name, "gl_MultiTexCoord")) {
                std::string numString = name.substr(16, 1);
                int textureUnit = atoi(numString.c_str());
                rd->setTexCoordArray(textureUnit, v);

            } else {
                alwaysAssertM(false, format("Built-in %s unsupported.\n", name.c_str()));
            }
        } else if (beginsWith(name, "g3d_")){ // Our "built-ins", let us say we will assign them even if the shader doesn't use them
            
            if(g3dAttributes.contains(name)){
                if(attributeInformationTable.containsKey(name)){
                    const ShaderProgram::AttributeDeclaration& decl = attributeInformationTable.get(name);
                    usedGeneric   = true;
                    debugAssertM(decl.name == name, format("%s != %s\n", decl.name.c_str(), name.c_str()));
                    rd->setVertexAttribArray(decl.location/* != 0 ? 0 : 1*/, v);
                }
            } else {
                alwaysAssertM(false, format("G3D attribute %s unimplemented.\n", name.c_str()));
            }

        } else  {
            usedGeneric   = true;
            if(attributeInformationTable.containsKey(name)){
                const ShaderProgram::AttributeDeclaration& decl = attributeInformationTable.get(name);
                rd->setVertexAttribArray(decl.location, v);
            } else {
                alwaysAssertM(false, format("Tried to assign attribute to %s, not used in the shader.\n", name.c_str()));
            }
 
            
        } 
        alwaysAssertM(!(usedGeneric && usedGLBuiltin), "Used GL builtin vertex attributes and generic attributes in the same shader.\n");
    }
    //TODO: Move this somewhere sensible
    glPatchParameteri(GL_PATCH_VERTICES,args.patchVertices);
}

void Shader2::compileAndBind(Args& args, RenderDevice* rd){
    compile(args);
    debugAssertGLOk();
    glUseProgram(shaderProgram());
    debugAssertGLOk();
    
    //bindStreamArgs(args, rd);

    setG3DArgs(args, rd);

    //debugPrintf("%s\n", args.toString().c_str());
    bindUniformArgs(args);
    debugAssertGLOk();
}

Shader2::Specification::Specification(const std::string& f0, const std::string& f1, 
        const std::string& f2, const std::string& f3, const std::string& f4) {
    const Array<std::string> filenames(f0, f1, f2, f3, f4);
    for(int i = 0; i < filenames.size(); ++i){
        const std::string fname = filenames[i];
        if(fname != ""){ // Skip blanks
            alwaysAssertM(fname.size() > 4, format("Invalid filename given to Shader2::Specification():\n%s", fname.c_str()));

            size_t extensionStart = fname.find_last_of('.');

            alwaysAssertM(extensionStart != std::string::npos && // Has a period
                (fname.size() - extensionStart) == 4,  // Extension is 3 characters

                format("Invalid filename given to Shader2::Specification():\n%s", fname.c_str()));
            const std::string extension = fname.substr(extensionStart + 1, 3);

            // Determine the stage
            ShaderStage stage;
            if(extension == "vrt" || extension == "vtx"){
                stage = VERTEX;
            } else if (extension == "ctl" || extension == "hul") {
                stage = TESSELLATION_CONTROL;
            } else if (extension == "evl" || extension == "dom") {
                stage = TESSELLATION_EVAL;
            } else if (extension == "geo") {
                stage = GEOMETRY;
            } else if (extension == "pix" || extension == "frg") {
                stage = PIXEL;
            } else {
                alwaysAssertM(true, format("Invalid filename given to Shader2::Specification():\n%s", fname.c_str()));
            }

            // Finally, set the source
            shaderStage[stage] = Source(FILE, fname);
            
        }
    }

}

Shader2::Ref Shader2::fromFiles(
        const std::string& f0, 
        const std::string& f1, 
        const std::string& f2, 
        const std::string& f3, 
        const std::string& f4){
    Specification s = Specification(f0, f1, f2, f3, f4);
    return create(s);
}



Shader2::Specification::Specification(){}

Shader2::Specification::Specification(const Any& any){
    /* if(any.containsKey("vertexFile")){
        vertex.val  = any["vertexFile"].string();
        vertex.type = FILE;
    } else if(any.containsKey("vertexString")){
        vertex.val = any["vertexString"].string();
    }
    if(any.containsKey("tessellationFile")){
        tessellation.val  = any["tessellationFile"].string();
        tessellation.type = FILE;
    } else if(any.containsKey("tessellationString")){
        tessellation.val = any["tessellationString"].string();
    }
    if(any.containsKey("tessellationControlFile")){
        tessellationControl.val  = any["tessellationControlFile"].string();
        tessellationControl.type = FILE;
    } else if(any.containsKey("tessellationControlString")){
        tessellationControl.val = any["tessellationControlString"].string();
    } 
    if(any.containsKey("geometryFile")){
        geometry.val  = any["geometryFile"].string();
        geometry.type = FILE;
    } else if(any.containsKey("geometryString")){
        geometry.val = any["geometryString"].string();
    } 
    if(any.containsKey("pixelFile")){
        pixel.val  = any["pixelFile"].string();
        pixel.type = FILE;
    } else if(any.containsKey("pixelString")){
        pixel.val = any["pixelString"].string();
    } */
}



static void readAndAppendShaderLog(const char* glInfoLog, std::string& messages, const std::string& name){
    int c = 0;
    // Copy the result to the output string, prepending the filename
    while (glInfoLog[c] != '\0') {
        messages += name;

        // Copy until the next newline or end of string
        std::string line;
        while (glInfoLog[c] != '\n' && glInfoLog[c] != '\r' && glInfoLog[c] != '\0') {
            line += glInfoLog[c];
            ++c;
        }

        if (beginsWith(line, "ERROR: ")) {
            // NVIDIA likes to preface messages with "ERROR: "; strip it off
            line = line.substr(7);

            if (beginsWith(line, "0:")) {
                // Now skip over the line number and wrap it in parentheses.
                line = line.substr(2);
                size_t i = line.find(':');
                if (i != std::string::npos) {
                    // Wrap the line number in parentheses
                    line = "(" + line.substr(0, i) + ")" + line.substr(i);
                } else {
                    // There was no line number, so just add a colon
                    line = ": " + line;
                }
            } else {
                line = ": " + line;
            }
        }

        messages += line;
        
        if (glInfoLog[c] == '\r' && glInfoLog[c + 1] == '\n') {
            // Windows newline
            messages += NEWLINE;
            c += 2;
        } else if (glInfoLog[c] == '\r' && glInfoLog[c + 1] != '\n') {
            // Dangling \r; treat it as a newline
            messages += NEWLINE;
            ++c;
        } else if (glInfoLog[c] == '\n') {
            // Newline
            messages += NEWLINE;
            ++c;
        }
    }
    
}



std::string stageName(int s){
    switch(s){

    case Shader2::VERTEX:
        return "Vertex";
    case Shader2::TESSELLATION_CONTROL:
        return "Tesselation Control";
    case Shader2::TESSELLATION_EVAL:
        return "Tesselation Evaluation";
    case Shader2::GEOMETRY:
        return "Geometry";
    case Shader2::PIXEL:
        return "Pixel";
    default:
        return "Invalid Stage";
    }
}


GLenum glShaderType(int s){
    switch(s){

    case Shader2::VERTEX:
        return GL_VERTEX_SHADER;
    case Shader2::TESSELLATION_CONTROL:
        return GL_TESS_CONTROL_SHADER;
    case Shader2::TESSELLATION_EVAL:
        return GL_TESS_EVALUATION_SHADER;
    case Shader2::GEOMETRY:
        return GL_GEOMETRY_SHADER;
    case Shader2::PIXEL:
        return GL_FRAGMENT_SHADER;
    default:
        alwaysAssertM(false, format("Invalid shader type %d given to glShaderType", s));
        return -1;
    }
}

Shader2::ShaderProgram::Ref Shader2::ShaderProgram::create(const Array<PreprocessedShaderSource>& preprocessedSource, const std::string& preambleAndMacroArgs){
    ShaderProgram::Ref s = new ShaderProgram();
    s->init(preprocessedSource, preambleAndMacroArgs);
    return s;
}


bool isNextToken(const std::string& macro, const std::string& code, int offset = 0){
    size_t macroOffset = code.find(macro, offset);
    if(offset == std::string::npos) return false;
    for(int i = offset; i < macroOffset; ++i){
        const char c = code[i];
        if(c != ' ' && c != '\t') return false;
    }
    return true;
}


size_t findPragmaWithSpaces(const std::string& macro, const std::string& code,  size_t offset = 0){
    if(beginsWith(code, "#")){
        if(isNextToken(macro, code, 1)) {
            return 0;
        }
    }

    do {
        offset = code.find("\n#", offset);
        if(offset != std::string::npos) {
            ++offset;
            if(isNextToken(macro, code, (int)offset + 1)){
                return offset;
            }
        } 

    } while(offset != std::string::npos);
    return offset;
}

size_t findLastPragmaWithSpaces(const std::string& macro, const std::string& code,  size_t offset = std::string::npos){
    

    do {
        offset = code.rfind("\n#", offset);
        if(offset != std::string::npos) {
            ++offset;
            if(isNextToken(macro, code, (int)offset + 1)){
                return offset;
            }
            offset -= 2;
        } 

    } while(offset != std::string::npos);

    if(beginsWith(code, "#")){
        if(isNextToken(macro, code, 1)) {
            return 0;
        }
    }


    return offset;
}

static int countNewlines(const std::string& s) { 
    int c = 0;
    for (int i = 0; i < (int)s.size(); ++i) {
        if (s[i] == '\n') {
            ++c;
        }
    }
    return c;
}

static std::string linePragma(int lineNumber = 1, std::string filename = ""){
    return format("#line %d \"%s\"\n", lineNumber, filename.c_str());
}

void Shader2::processIncludes(const std::string& dir, std::string& code){
    // Look for #include immediately after a newline.  If it is inside
    // a #IF or a block comment, it will still be processed, but
    // single-line comments will properly disable it.
    bool foundPound = false;
    do {
        foundPound = false;
        size_t includeLoc = findPragmaWithSpaces("include", code);
        size_t lineLoc    = findLastPragmaWithSpaces("line", code, includeLoc);
        if (includeLoc != std::string::npos) {
            // Remove this line
            size_t includeEnd = code.find("\n", includeLoc + 1);
            if (includeEnd == std::string::npos) {
                includeEnd = code.size();
            }
  
            const std::string& includeLine = code.substr(includeLoc, includeEnd - includeLoc + 1);
            std::string includedFilename;
            TextInput t (TextInput::FROM_STRING, includeLine);
            t.readSymbols("#", "include");
            includedFilename = t.readString();            

            if (! beginsWith(includedFilename, "/")) {
                includedFilename = pathConcat(dir, includedFilename);
            }
            if (! FileSystem::exists(includedFilename)) {
                includedFilename = System::findDataFile(includedFilename);
            }
            // TODO: exception handling
            std::string includedFile = readWholeFile(includedFilename);
            if (! endsWith(includedFile, "\n")) {
                includedFile += "\n";
            }

            // Find the current filename
            size_t linePragmaEnd            = code.find("\n", lineLoc + 1);
            const std::string& lastLinePragma   = code.substr(lineLoc, linePragmaEnd - lineLoc + 1);
            TextInput tlp (TextInput::FROM_STRING, lastLinePragma);
            tlp.readSymbols("#", "line");
            int lastLineNumber      = tlp.readInteger();
            std::string lastFile    = tlp.readString();

            // # of newLines between the include pragma and the closest line pragma before it
            int linesSinceLastLineNumber    = countNewlines(code.substr(linePragmaEnd + 1, includeLoc - linePragmaEnd - 1));


            code = code.substr(0, includeLoc) + linePragma(1, includedFilename) + includedFile 
                + linePragma(lastLineNumber + linesSinceLastLineNumber, lastFile) + code.substr(includeEnd);
            foundPound = true;
        }

    } while (foundPound);
}

bool processVersionOrs(std::string& versionLine){
    size_t index    = 0;
    size_t lastIndex   = 0;
    Array<std::string> versionPhrases;
    // Split the line at the "or"s: "# version 140 or 220 or 330" becomes {"# version 140","220","330"}
    while(versionLine.find(" or ", index) != std::string::npos) {
        index = versionLine.find(" or ", index);
        versionPhrases.append(versionLine.substr(lastIndex, index - lastIndex));
        index += 4;
        lastIndex = index;
    } 
    //TODO: respect comments
    versionPhrases.append(versionLine.substr(lastIndex, versionLine.find("\n", index) - lastIndex));
    Array<int> versionIntegers;
    for(int i = 0; i < versionPhrases.size(); ++i){
        TextInput ti (TextInput::FROM_STRING, versionPhrases[i]);
        // #version and version are optional except for the #version in the first phrase
        if(ti.peek().string() == "#"){ 
            ti.readSymbols("#", "version");
        } else if (ti.peek().string() == "version"){
            ti.readSymbol("version");
        }
        versionIntegers.append(ti.readInteger());
    }

    static int validGLSLVersions[] = {110,120,130,140,150,330,400,410,420};
    int version = 0;
    // Use the highest version specified
    for(int i = sizeof(validGLSLVersions)/sizeof(int) - 1; i >= 0 ; --i){
        if(versionIntegers.contains(validGLSLVersions[i])){
            version = validGLSLVersions[i];
            break;
        }
    }
    versionLine = format("#version %d\n", version);
    //TODO: thread in error handling code
    return version != 0;


}

bool Shader2::processVersion(std::string& code, std::string& versionLine){
    size_t i = findPragmaWithSpaces("version", code);
        
    if (i != std::string::npos) {
        // Remove this line
        size_t end = code.find("\n", i + 1);
        if (end == std::string::npos) {
            end = code.size();
        }
        versionLine = code.substr(i, end - i + 1);
        processVersionOrs(versionLine);

        code = code.substr(0, i) + "\n" + code.substr(end); // Reinsert \n to avoid changing line numbers
        return true;
    } else {
        // Insert #version 130
        versionLine = "#version 130\n";
        return false;
    }
    
}


bool Shader2::ShaderProgram::containsNonDummyUniform(const std::string& name) {
    return declarationTable.containsKey(name) && declarationTable.get(name).dummy == false;
}

void Shader2::setG3DArgs(Args& args, RenderDevice* renderDevice){
    const CoordinateFrame& o2w = renderDevice->objectToWorldMatrix();
    const CoordinateFrame& c2w = renderDevice->cameraToWorldMatrix();
    const ShaderProgram::Ref& p = m_shaderProgram; 
    
    // Bind matrices
    if (p->containsNonDummyUniform("g3d_ObjectToWorldMatrix")) {
        args.setUniform("g3d_ObjectToWorldMatrix", o2w);
    }

    if (p->containsNonDummyUniform("g3d_ModelViewProjectionMatrix")){
        args.setUniform("g3d_ModelViewProjectionMatrix", renderDevice->modelViewProjectionMatrix());
    }

    if (p->containsNonDummyUniform("g3d_CameraToWorldMatrix")) {
        args.setUniform("g3d_CameraToWorldMatrix", c2w);
    }

    if (p->containsNonDummyUniform("g3d_ObjectToWorldNormalMatrix")) {
        args.setUniform("g3d_ObjectToWorldNormalMatrix", o2w.rotation);
    }

    if (p->containsNonDummyUniform("g3d_ObjectToCameraNormalMatrix")) {
        args.setUniform("g3d_ObjectToCameraNormalMatrix", c2w.inverse().rotation * o2w.rotation);
    }

    if (p->containsNonDummyUniform("g3d_CameraToObjectNormalMatrix")) {
        args.setUniform("g3d_CameraToObjectNormalMatrix", (c2w.inverse().rotation * o2w.rotation).inverse());
    }

    if (p->containsNonDummyUniform("g3d_WorldToObjectNormalMatrix")) {
        args.setUniform("g3d_WorldToObjectNormalMatrix", o2w.rotation.transpose());
    }

    if (p->containsNonDummyUniform("g3d_WorldToObjectMatrix")) {
        args.setUniform("g3d_WorldToObjectMatrix", o2w.inverse());
    }

    if (p->containsNonDummyUniform("g3d_WorldToCameraMatrix")) {
        const CFrame& c = c2w.inverse();
        args.setUniform("g3d_WorldToCameraMatrix", c);
        //debugPrintf("Binding WorldToCameraMatrix: %s\n", c.toXYZYPRDegreesString().c_str());
    }

    if (p->containsNonDummyUniform("g3d_WorldToCameraNormalMatrix")) {
        const CFrame& c = c2w.inverse();
        args.setUniform("g3d_WorldToCameraNormalMatrix", c.rotation);
        //debugPrintf("Binding WorldToCameraMatrix: %s\n", c.toXYZYPRDegreesString().c_str());
    }

    if (p->containsNonDummyUniform("g3d_InvertY")) {
        args.setUniform("g3d_InvertY", renderDevice->invertY());
    }
}


void Shader2::g3dPreprocessor(const std::string& dir, PreprocessedShaderSource& source){
    std::string& code           = source.preprocessedCode;
    std::string& versionString  = source.versionString;
    const std::string& name     = source.filename;
    std::string& insertString   = source.g3dInsertString;

    // G3D Preprocessor
    // Handle #include directives first, since they may affect
    // what preprocessing is needed in the code. 
    code = "#line 1 \"" + name + "\"\n" + code;
    Shader2::processIncludes(dir, code);        

    // Standard uniforms.  We'll add custom ones to this below
    std::string uniformString = 
        STR(uniform mat4x3 g3d_WorldToObjectMatrix;
            uniform mat4x3 g3d_ObjectToWorldMatrix;
            uniform mat3   g3d_WorldToObjectNormalMatrix;
            uniform mat3   g3d_ObjectToWorldNormalMatrix;
            uniform mat3   g3d_ObjectToCameraNormalMatrix;
            uniform mat3   g3d_CameraToObjectNormalMatrix;
            uniform mat4x3 g3d_WorldToCameraMatrix;
            uniform mat4x3 g3d_CameraToWorldMatrix;
            uniform int    g3d_NumLights;
            uniform int    g3d_NumTextures;
            uniform vec4   g3d_ObjectLight0;
            uniform vec4   g3d_WorldLight0;
            uniform bool   g3d_InvertY;
            uniform mat4   g3d_ModelViewProjectionMatrix;
            uniform mat3   g3d_WorldToCameraNormalMatrix;
            );

    processVersion(code, versionString);

    // #defines we'll prepend onto the shader
    std::string defineString;
        
    switch (GLCaps::enumVendor()) {
    case GLCaps::ATI:
        defineString += "#define G3D_ATI\n";
        break;

    case GLCaps::NVIDIA:
        defineString += "#define G3D_NVIDIA\n";
        break;

    case GLCaps::MESA:
        defineString += "#define G3D_MESA\n";
        break;

    default:;
    }

#       ifdef G3D_OSX 
        defineString += "#define G3D_OSX\n";
#       elif defined(G3D_WINDOWS)
        defineString += "#define G3D_WINDOWS\n";
#       elif defined(G3D_LINUX)
        defineString += "#define G3D_LINUX\n";
#       elif defined(G3D_FREEBSD)
        defineString += "#define G3D_FREEBSD\n";
#       endif

#       if defined(G3D_WIN32)
        defineString += "#define G3D_WIN32\n";
#       endif

#       if defined(G3D_64BIT)
        defineString += "#define G3D_64BIT\n";
#       endif

    Array<std::string> extensions("GL_EXT_gpu_shader4",
                                    "GL_ARB_gpu_shader5");

    /* TODO: check if we actually need this
    for (int i = 0; i < extensions.length(); ++i) {
        if (GLCaps::supports(extensions[i])) {
            defineString += "#define " + extensions[i] + "1\n";
        }
    }*/
                
    // Replace g3d_size and g3d_invSize with corresponding magic names
    //replaceG3DSize(_code, uniformString);
            
    //m_usesG3DIndex = replaceG3DIndex(_code, defineString, samplerMappings, secondPass);
        
    insertString = defineString + uniformString + "\n";
    insertString +=  "// End of G3D::Shader2 inserted code\n";

    code += "\n";

}

void Shader2::load(){
    for(int s = 0; s < STAGE_COUNT; ++s){
        debugAssertGLOk();
        
        const Source& source = m_specification.shaderStage[s];

        PreprocessedShaderSource pSource;
        std::string& code = pSource.preprocessedCode;
        std::string& name = pSource.filename;
        std::string dir = "";

        // Read the code into a string
        if(source.type == STRING){
            code = source.val;     
        } else {
            // TODO: catch exception?
            name = source.val;
            if(name != ""){
                code = readWholeFile(name);
                dir = filenamePath(name);
            } 
        }
        
        debugAssertGLOk();
        // If it's empty, there's nothing to preprocess
        if(code != ""){
            g3dPreprocessor(dir, pSource);
        }
        m_preprocessedSource.append(pSource);
    }
}



void Shader2::ShaderProgram::init(const Array<PreprocessedShaderSource>& pss, const std::string& preambleAndMacroArgs){
    ok = true;
    debugAssertGLOk();
    if (! GLCaps::supports_GL_ARB_shader_objects()) {
        messages = "This graphics card does not support GL_ARB_shader_objects.";
        ok = false;
        return;
    }
    debugAssertGLOk();
    compile(pss, preambleAndMacroArgs);
    debugAssertGLOk();
    if(ok){
       link();
    }
    debugAssertGLOk();
    if(ok){
        computeUniformTable();
        debugAssertGLOk();
        addUniformsFromSource(pss);
        debugAssertGLOk();
    }
    if(ok){
        computeVertexAttributeTable();
        debugAssertGLOk();
    }
    logPrintf("%s\n", messages.c_str());
}

void Shader2::ShaderProgram::link(){
    glProgramObject = glCreateProgram();
    debugAssertGLOk();
    // Attach
    for(int s = 0; s < STAGE_COUNT; ++s){
        if(glShaderObject[s]){
            glAttachShader(glProgramObject, glShaderObject[s]);
        }
        debugAssertGLOk();
    }

    // Link
    glLinkProgram(glProgramObject);
    debugAssertGLOk();


    // Read back messages
    GLint linked;
    glGetProgramiv(glProgramObject, GL_LINK_STATUS, &linked);
    debugAssertGLOk();

    GLint maxLength = 0, length = 0;
    glGetProgramiv(glProgramObject, GL_INFO_LOG_LENGTH, &maxLength);
    GLchar* pInfoLog = (GLchar *)malloc(maxLength * sizeof(GLcharARB));
    glGetProgramInfoLog(glProgramObject, maxLength, &length, pInfoLog);
    debugAssertGLOk();
    messages += std::string("Linking\n") + std::string(pInfoLog) + "\n";
    ok = ok && (linked == GL_TRUE);
}

void Shader2::ShaderProgram::compile(const Array<PreprocessedShaderSource>& pss, const std::string& preambleAndMacroArgs){
    
    debugAssertGLOk();
    for(int s = 0; s < STAGE_COUNT; ++s){
        const PreprocessedShaderSource& pSource = pss[s];
        if(pSource.preprocessedCode != ""){
            std::string code = pSource.versionString + preambleAndMacroArgs + 
                pSource.g3dInsertString + pSource.preprocessedCode;

            GLint compiled = GL_FALSE;
            GLuint& glShader = glShaderObject[s];
            glShader = glCreateShader(glShaderType(s));
            debugAssertGLOk();
            // Compile the shader
            GLint length = (GLint)code.length();
            const GLchar* codePtr = static_cast<const GLchar*>(code.c_str());
            // Show the preprocessed code: TODO remove, this is just for testing
            debugPrintf("\"%s\"\n", code.c_str());

            int count = 1;
            glShaderSource(glShader, count, &codePtr, &length);
            debugAssertGLOk();
            glCompileShader(glShader);
            debugAssertGLOk();
            glGetShaderiv(glShader, GL_COMPILE_STATUS, &compiled);
            debugAssertGLOk();

            // Read the result of compilation
            GLint maxLength;
            glGetShaderiv(glShader, GL_INFO_LOG_LENGTH, &maxLength);

            debugAssertGLOk();

            GLchar* pInfoLog = (GLchar *)malloc(maxLength * sizeof(GLchar));
            glGetShaderInfoLog(glShader, maxLength, &length, pInfoLog);
            debugAssertGLOk();
    
            readAndAppendShaderLog(pInfoLog, messages, pSource.filename);
            debugAssertGLOk();
            free(pInfoLog);
            ok &= (compiled == GL_TRUE);
        } else {
            // No code to compile from, so the shader object does not exist
            glShaderObject[s] = 0; 
        }
    
    }
    

}

void Shader2::ShaderProgram::cleanDeclarationTables(){
    //declarationTable.clear();
    //attributeTable.clear();
}

void Shader2::compile(const Args& args){
    const std::string& preambleAndMacroString = args.preambleAndMacroString();
    if(m_compilationCache.containsKey(preambleAndMacroString)){ 
        // In cache, no need for recompilation
        m_shaderProgram = m_compilationCache.get(preambleAndMacroString);
        
        //debugPrintf("Found shader in cache for preamble and macro string %s.\n", preambleAndMacroString.c_str());
    } else {
        debugAssertGLOk();
        m_shaderProgram = ShaderProgram::create(m_preprocessedSource, preambleAndMacroString);
        m_ok = m_shaderProgram->ok;

        debugAssertGLOk();
        debugPrintf("Unable to find shader in cache for preamble and macro string %s.\n", preambleAndMacroString.c_str());
        m_compilationCache.set(preambleAndMacroString, m_shaderProgram);
        debugPrintf(m_shaderProgram->messages.c_str());
        if(!m_ok){
            debugPrintf(m_shaderProgram->messages.c_str());
            if(s_failureBehavior == PROMPT){
                const int cDebug  = 0;
                const int cRetry = 1;
                const int cExit   = 2;

                static char* options[] = {"Debug", "Retry", "Exit"};
            
                int userAction = prompt("Shader Compilation Failed",
                                    m_shaderProgram->messages.c_str(),
                                    (const char**)options,
                                    3,
                                    true
                                 );	

                switch(userAction){
                case cDebug:
                    rawBreak();
                    break;
                case cRetry:
                    retry(args);
                    break;
                case cExit:
                    exit(-1);
                    break;
                }

            } else if (s_failureBehavior == EXCEPTION){
                alwaysAssertM(m_ok, format("Shader Compilation Error (see log): \n %s\n", m_shaderProgram->messages.c_str()));
            }
        }
        
        
    }
}


Shader2::Source::Source(const std::string& value){
    // All valid shader code has a semicolon, no filenames do if you're sane.
    alwaysAssertM(value.find(';') != std::string::npos, format("The Source(string) constructor only \
        accepts GLSL code, not filenames. The passed in string was:\n %s \nIf this looks like code, look \
        for missing semicolons (all valid code should have at least one).\nIf this is a filename, use \
        the Source(SourceType, string) constructor instead, with a SourceType of FILE.\n", value.c_str()));
    type = STRING;
    val = value;
}

Shader2::Source::Source(SourceType t, const std::string& value) :
    type(t), val(value){}

Shader2::Source::Source(){

}

void Shader2::setFailureBehavior(FailureBehavior f){
    s_failureBehavior = f;
}
    

void Shader2::reload(){
    m_ok = true;
    m_compilationCache.clear();
    m_shaderProgram = NULL;
    m_preprocessedSource.clear();

    load();
    debugPrintf("SPECIFICATION:\n%s\n", m_specification.shaderStage[4].val.c_str());
}

void Shader2::retry(const Args& args){
    reload();
    compile(args);
}

bool Shader2::ok() const{
    return m_ok;
}

Shader2::Ref Shader2::create(const Specification& s){
    Shader2::Ref shader = new Shader2(s);
    shader->load();
    return shader;
}

    



}
