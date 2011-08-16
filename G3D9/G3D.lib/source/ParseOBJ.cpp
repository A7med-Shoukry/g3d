/**
 \file G3D/source/ParseOBJ.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-16
 \edited  2011-08-22
 
 Copyright 2000-2011, Morgan McGuire.
 All rights reserved.
*/
#include "G3D/ParseOBJ.h"
#include "G3D/BinaryInput.h"
#include "G3D/FileSystem.h"
#include "G3D/stringutils.h"
#include "G3D/TextInput.h"

namespace G3D {


void ParseOBJ::parse(const char* ptr, int len, const std::string& basePath) {
    vertexArray.clear();
    normalArray.clear();
    texCoordArray.clear();
    groupTable.clear();
    m_basePath = "";
    m_currentGroup = NULL;
    m_currentMesh = NULL;
    m_currentMaterial = NULL;

    m_basePath = basePath;

    nextCharacter = ptr;
    remainingCharacters = len;
    m_line = 1;

    while (remainingCharacters > 0) {
        // Process leading whitespace
        maybeReadWhitespace();

        const Command command = readCommand();
        processCommand(command);
    }        
}


void ParseOBJ::parse(BinaryInput& bi, const std::string& basePath) {
    m_filename = bi.getFilename();

    std::string bp = basePath;
    if (bp == "<AUTO>") {
        bp = FilePath::parent(FileSystem::resolve(m_filename));
    }
    
    

    parse((const char*)bi.getCArray() + bi.getPosition(),
          bi.getLength() - bi.getPosition(), bp);
}


ParseMTL::Material::Ref ParseOBJ::getMaterial(const std::string& materialName) {
    bool created = false;
    ParseMTL::Material::Ref& m =
        m_currentMaterialLibrary.materialTable.getCreate(materialName);

    if (created) {
        m = ParseMTL::Material::create();
        debugPrintf("Warning: missing material %s used.\n", materialName.c_str());
    }
    return m;
}


bool ParseOBJ::maybeReadWhitespace() {
    bool changedLines = false;

    while (remainingCharacters > 0) {
        switch (*nextCharacter) {
        case '\n':
        case '\r':
            {
                char c = *nextCharacter;
                consumeCharacter();
                ++m_line;
                changedLines = true;
                if ((remainingCharacters > 0) && (c != *nextCharacter) && 
                    ((*nextCharacter == '\r') || (*nextCharacter == '\n'))) {
                    // This is part of a two-character, e.g., Mac or
                    // Windows.  Consume the next character as well.
                    consumeCharacter();
                }
            }
            break;

        case ' ':
        case '\t':
            // Consume whitespace
            consumeCharacter();
            break;

        case '#':
            // Consume comment
            readUntilNewline();
            // Don't consume the newline; we'll catch it on the next
            // iteration
            break;

        default:
            return changedLines;
        }
    }

    return true;
}


ParseOBJ::Command ParseOBJ::readCommand() {
    if (remainingCharacters == 0) {
        return UNKNOWN;
    }

    // Explicit finite automata parser
    switch (*nextCharacter) {
    case 'f':
        consumeCharacter();
        if (isSpace(*nextCharacter)) {
            return FACE;
        } else {
            return UNKNOWN;
        }
        break;

    case 'v':
        consumeCharacter();
        switch (*nextCharacter) {
        case ' ':
        case '\t':
            return VERTEX;

        case 'n':
            consumeCharacter();
            if (isSpace(*nextCharacter)) {
                return NORMAL;
            } else {
                return UNKNOWN;
            }
            break;

        case 't':
            consumeCharacter();
            if (isSpace(*nextCharacter)) {
                return TEXCOORD;
            } else {
                return UNKNOWN;
            }
            break;

        default:
            return UNKNOWN;
        }
        break;

    case 'm':
        if ((remainingCharacters > 6) && (memcmp(nextCharacter, "mtllib", 6) == 0)) {
            nextCharacter += 6; remainingCharacters -= 6;
            if (isSpace(*nextCharacter)) {
                return MTLLIB;
            } else {
                return UNKNOWN;
            }
        } else {
            return UNKNOWN;
        }
        break;

    case 'u':
        if ((remainingCharacters > 6) && (memcmp(nextCharacter, "usemtl", 6) == 0)) {
            nextCharacter += 6; remainingCharacters -= 6;
            if (isSpace(*nextCharacter)) {
                return USEMTL;
            } else {
                return UNKNOWN;
            }
        } else {
            return UNKNOWN;
        }
        break;

    case 'g':
        consumeCharacter();
        if (isSpace(*nextCharacter)) {
            return GROUP;
        } else {
            return UNKNOWN;
        }
        break;

    default:
        return UNKNOWN;
    }
}


void ParseOBJ::readFace() {
    // Ensure that we have a material
    if (m_currentMaterial.isNull()) {
        m_currentMaterial = m_currentMaterialLibrary.materialTable["default"];
    }

    // Mnsure that we have a group
    if (m_currentGroup.isNull()) {
        // Create a group named "default", per the OBJ specification
        m_currentGroup = Group::create();
        m_currentGroup->name = "default";
        groupTable.set(m_currentGroup->name, m_currentGroup);

        // We can't have a mesh without a group, but conservatively reset this anyway
        m_currentMesh = NULL;
    }

    // Ensure that we have a mesh
    if (m_currentMesh.isNull()) {
        bool created = false;
        Mesh::Ref& m = m_currentGroup->meshTable.getCreate(m_currentMaterial, created);

        if (created) {
            m = Mesh::create();
            m->material = m_currentMaterial;
        }
        m_currentMesh = m;
    }

    Face& face = m_currentMesh->faceArray.next();

    const int vertexArraySize   = vertexArray.size();
    const int texCoordArraySize = texCoordArray.size();
    const int normalArraySize   = normalArray.size();

    // Consume leading whitespace
    bool done = maybeReadWhitespace();
    while (! done) {
        Index& index = face.next();

        // Read index
        index.vertex = readInt();
        if (index.vertex > 0) {
            // Make 0-based
            --index.vertex;
        } else {
            // Negative; make relative to the current end of the array.
            // -1 will be the last element, so just add the size of the array.
            index.vertex += vertexArraySize;
        }

        if ((remainingCharacters > 0) && (*nextCharacter == '/')) {
            // Read the slash
            consumeCharacter();

            if (remainingCharacters > 0) {
                if (*nextCharacter == '/') {
                    // No texcoord index
                    consumeCharacter();
                } else {
                    // texcoord index
                    index.texCoord = readInt();
                    if (index.texCoord > 0) {
                        // Make 0-based
                        --index.texCoord;
                    } else {
                        // Negative; make relative to the current end
                        // of the array.  -1 will be the last element,
                        // so just add the size of the array.
                        index.texCoord += texCoordArraySize;
                    }
                }

                if ((remainingCharacters > 0) && (*nextCharacter == '/')) {
                    // Consume the slash
                    consumeCharacter();

                    // normal index
                    index.normal = readInt();
                    if (index.normal > 0) {
                        // Make 0-based
                        --index.normal;
                    } else {
                        // Negative; make relative to the current
                        // end of the array.  -1 will be the last
                        // element, so just add the size of the
                        // array.
                        index.normal += normalArraySize;
                    }       
                }
            }
        }

        // Read remaining whitespace
        done = maybeReadWhitespace();
    }
}


void ParseOBJ::processCommand(const Command command) {
    switch (command) {
    case VERTEX:
        maybeReadWhitespace();
        vertexArray.append(readVector3());
        // Consume anything else on this line
        readUntilNewline();
        break;

    case TEXCOORD:
        maybeReadWhitespace();
        texCoordArray.append(readVector2());
        // Consume anything else on this line
        readUntilNewline();
        break;

    case NORMAL:
        maybeReadWhitespace();
        normalArray.append(readVector3());
        // Consume anything else on this line
        readUntilNewline();
        break;

    case FACE:
        readFace();
        // Faces consume newlines by themselves
        break;

    case GROUP:
        {
            // Change group
            std::string groupName = readName();

            Group::Ref& g = groupTable.getCreate(groupName);

            if (g.isNull()) {
                // Newly created
                g = Group::create();
                g->name = groupName;
            }

            m_currentGroup = g;
        }
        // Consume anything else on this line
        readUntilNewline();
        break;

    case USEMTL:
        {
            // Change the mesh within the group
            const std::string& materialName = readName();
            m_currentMaterial = getMaterial(materialName);

            // Force re-obtaining or creating of the appropriate mesh
            m_currentMesh = NULL;
        }
        // Consume anything else on this line
        readUntilNewline();
        break;

    case MTLLIB:
        {
            // Specify material library 
            std::string mtlFilename = readName();
            mtlFilename = FilePath::concat(m_basePath, mtlFilename);

            TextInput ti2(mtlFilename);
            m_currentMaterialLibrary.parse(ti2);
        }
        // Consume anything else on this line
        readUntilNewline();
        break;

    case UNKNOWN:
        // Nothing to do
        readUntilNewline();
        break;
    }
}



#if 0
void ParseOBJ::parse(TextInput& ti, const std::string& basePath) {

    // Clear state
    vertexArray.clear();
    normalArray.clear();
    texCoordArray.clear();
    groupTable.clear();
    m_basePath = "";
    m_currentGroup = NULL;
    m_currentMesh = NULL;
    m_currentMaterial = NULL;

    m_basePath = basePath;
    if (m_basePath == "<AUTO>") {
        m_basePath = FilePath::parent(FileSystem::resolve(ti.filename()));
    }

    TextInput::Settings set;
    set.cppBlockComments = false;
    set.cppLineComments = false;
    set.otherCommentCharacter = '#';
    set.generateNewlineTokens = true;
    // Don't let ".#" parse as a float special, since '#' starts a comment.
    set.msvcFloatSpecials = false;

    ti.pushSettings(set);

    while (ti.hasMore()) {
        // Consume comments/newlines
        while (ti.hasMore() && (ti.peek().type() == Token::NEWLINE)) {
            // Consume the newline
            ti.read();
        }

        if (! ti.hasMore()) {
            break;
        }

        // Process one line
        const std::string& cmd = ti.readSymbol();
        processCommand(ti, cmd);

        // Read until the end of the line
        while (ti.hasMore() && (ti.read().type() != Token::NEWLINE));
    }

    ti.popSettings();
}


inline static Vector3 readVector3(TextInput& ti) {
    Vector3 v;
    for (int i = 0; i < 3; ++i) { v[i] = ti.readNumber(); }
    return v;
}


inline static Vector2 readVector2(TextInput& ti) {
    Vector2 v;
    for (int i = 0; i < 2; ++i) { v[i] = ti.readNumber(); }
    return v;
}


ParseMTL::Material::Ref ParseOBJ::getMaterial(const std::string& materialName) {
    bool created = false;
    ParseMTL::Material::Ref& m = m_currentMaterialLibrary.materialTable.getCreate(materialName);
    if (created) {
        m = ParseMTL::Material::create();
        debugPrintf("Warning: missing material %s used.\n", materialName.c_str());
    }
    return m;
}


void ParseOBJ::processCommand(TextInput& ti, const std::string& cmd) {

    if (cmd == "mtllib") {

        // Specify material library 
        std::string mtlFilename = trimWhitespace(ti.readUntilNewlineAsString());
        mtlFilename = FilePath::concat(m_basePath, mtlFilename);

        TextInput ti2(mtlFilename);
        m_currentMaterialLibrary.parse(ti2);

    } else if (cmd == "g") {
        // Change group
        std::string groupName = trimWhitespace(ti.readUntilNewlineAsString());

        Group::Ref& g = groupTable.getCreate(groupName);

        if (g.isNull()) {
            // Newly created
            g = Group::create();
            g->name = groupName;
        }

        m_currentGroup = g;

    } else if (cmd == "usemtl") {

        // Change the mesh within the group
        const std::string& materialName = trimWhitespace(ti.readUntilNewlineAsString());        
        m_currentMaterial = getMaterial(materialName);

        // Force re-obtaining or creating of the appropriate mesh
        m_currentMesh = NULL;

    } else if (cmd == "v") {

        vertexArray.append(readVector3(ti));

    } else if (cmd == "vt") {

        texCoordArray.append(readVector2(ti));

    } else if (cmd == "vn") {
        
        normalArray.append(readVector3(ti));

    } else if (cmd == "f") {

        processFace(ti);

    }
}


void ParseOBJ::processFace(TextInput& ti) {
        // Ensure that we have a material
    if (m_currentMaterial.isNull()) {
        m_currentMaterial = m_currentMaterialLibrary.materialTable["default"];
    }

    // Mnsure that we have a group
    if (m_currentGroup.isNull()) {
        // Create a group named "default", per the OBJ specification
        m_currentGroup = Group::create();
        m_currentGroup->name = "default";
        groupTable.set(m_currentGroup->name, m_currentGroup);

        // We can't have a mesh without a group, but conservatively reset this anyway
        m_currentMesh = NULL;
    }

    // Ensure that we have a mesh
    if (m_currentMesh.isNull()) {
        bool created = false;
        Mesh::Ref& m = m_currentGroup->meshTable.getCreate(m_currentMaterial, created);

        if (created) {
            m = Mesh::create();
            m->material = m_currentMaterial;
        }
        m_currentMesh = m;
    }


    Face& face = m_currentMesh->faceArray.next();

    const int vertexArraySize   = vertexArray.size();
    const int texCoordArraySize = texCoordArray.size();
    const int normalArraySize   = normalArray.size();

    // Read each vertex
    while (ti.hasMore() && (ti.peek().type() != Token::NEWLINE)) {

        // Read the index, making absolute and 0-based
        Index& index = face.next();

        index.vertex = ti.readInteger();
        if (index.vertex > 0) {
            // Make 0-based
            --index.vertex;
        } else {
            // Negative; make relative to the current end of the array.
            // -1 will be the last element, so just add the size of the array.
            index.vertex += vertexArraySize;
        }

        if (ti.peek().type() == Token::SYMBOL) {
            // Optional texcoord and normal
            ti.readSymbol("/");
            if (ti.peek().type() == Token::NUMBER) {

                index.texCoord = ti.readInteger();
                if (index.texCoord > 0) {
                    // Make 0-based
                    --index.texCoord;
                } else {
                    // Negative; make relative to the current end of the array.
                    // -1 will be the last element, so just add the size of the array.
                    index.texCoord += texCoordArraySize;
                }
            }

            if (ti.peek().type() == Token::SYMBOL) {
                ti.readSymbol("/");
                if (ti.peek().type() == Token::NUMBER) {

                    index.normal = ti.readInteger();
                    if (index.normal > 0) {
                        // Make 0-based
                        --index.normal;
                    } else {
                        // Negative; make relative to the current end of the array.
                        // -1 will be the last element, so just add the size of the array.
                        index.normal += normalArraySize;
                    }                
                }
            } // if has normals
        } // if has texcoords
    } // while more vertices

}
#endif
} // namespace G3D
