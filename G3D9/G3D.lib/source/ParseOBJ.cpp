/**
 \file G3D/source/ParseMTL.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-16
 \edited  2011-07-22
 
 Copyright 2000-2011, Morgan McGuire.
 All rights reserved.
*/
#include "G3D/ParseOBJ.h"
#include "G3D/TextInput.h"
#include "G3D/FileSystem.h"
#include "G3D/stringutils.h"

namespace G3D {

void ParseOBJ::parse(TextInput& ti, const std::string& basePath) {

    // TODO: clear state

    m_basePath = basePath;
    if (m_basePath == "<AUTO>") {
        m_basePath = FilePath::parent(FileSystem::resolve(ti.filename()));
    }

    TextInput::Settings set;
    set.cppBlockComments = false;
    set.cppLineComments = false;
    set.otherCommentCharacter = '#';
    set.generateNewlineTokens = true;

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

        m_currentMaterialLibrary.parse(TextInput(mtlFilename));

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

    // Read each vertex
    while (ti.hasMore() && (ti.peek().type() != Token::NEWLINE)) {

        // Read the index, making absolute and 0-based
        Index& index = face.next();

        index.vertex = ti.readNumber();
        if (index.vertex > 0) {
            // Make 0-based
            --index.vertex;
        } else {
            // Negative; make relative to the current end of the array.
            // -1 will be the last element, so just add the size of the array.
            index.vertex += vertexArray.size();
        }

        if (ti.peek().type() == Token::SYMBOL) {
            // Optional texcoord and normal
            ti.readSymbol("/");
            if (ti.peek().type() == Token::NUMBER) {

                index.texCoord = ti.readNumber();
                if (index.texCoord > 0) {
                    // Make 0-based
                    --index.texCoord;
                } else {
                    // Negative; make relative to the current end of the array.
                    // -1 will be the last element, so just add the size of the array.
                    index.texCoord += texCoordArray.size();
                }
            }

            if (ti.peek().type() == Token::SYMBOL) {
                ti.readSymbol("/");
                if (ti.peek().type() == Token::NUMBER) {

                    index.normal = ti.readNumber();
                    if (index.normal > 0) {
                        // Make 0-based
                        --index.normal;
                    } else {
                        // Negative; make relative to the current end of the array.
                        // -1 will be the last element, so just add the size of the array.
                        index.normal += normalArray.size();
                    }                
                }
            } // if has normals
        } // if has texcoords
    } // while more vertices

}

} // namespace G3D
