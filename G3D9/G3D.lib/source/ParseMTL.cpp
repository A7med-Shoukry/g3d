/**
 \file G3D/source/ParseMTL.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-16
 \edited  2011-07-22
 
 Copyright 2000-2011, Morgan McGuire.
 All rights reserved.
*/
#include "G3D/ParseMTL.h"
#include "G3D/TextInput.h"
#include "G3D/stringutils.h"
#include "G3D/FileSystem.h"
#include "G3D/Log.h"

namespace G3D {

void ParseMTL::parse(TextInput& ti, const std::string& basePath) {
    materialTable.clear();

    TextInput::Settings set;
    set.cppBlockComments = false;
    set.cppLineComments = false;
    set.otherCommentCharacter = '#';
    set.generateNewlineTokens = true;
    ti.pushSettings(set);

    m_basePath = basePath;
    if (m_basePath == "<AUTO>") {
        m_basePath = FilePath::parent(FileSystem::resolve(ti.filename()));
    }

    while (ti.hasMore()) {
        // Consume comments/newlines
        while (ti.hasMore() && (ti.peek().type() == Token::NEWLINE)) {
            // Consume the newline
            ti.read();
        }

        // Process one line
        const std::string& cmd = ti.readSymbol();

        processCommand(ti, cmd);

        // Read until the end of the line if this line did not consume it
        while (ti.hasMore() && (ti.read().type() != Token::NEWLINE));
    }

    if (! materialTable.containsKey("default")) {
        materialTable.set("default", Material::create());
    }

    ti.popSettings();
}


static std::string removeLeadingSlash(const std::string& s) {
    if (s.length() > 0 && isSlash(s[0])) {
        return s.substr(1);
    } else {
        return s;
    }
}

void ParseMTL::processCommand(TextInput& ti, const std::string& cmd) {

    if (cmd == "newmtl") {
        // Create a new material
        m_currentMaterial = Material::create();
        m_currentMaterial->name = trimWhitespace(ti.readUntilNewlineAsString());
        materialTable.set(m_currentMaterial->name, m_currentMaterial);

    } else if (m_currentMaterial.isNull()) {
            logPrintf("Warning: encountered command with null material\n");
    } else if (cmd == "d") {
        // "dissolve"; alpha on range [0,1]
        if (ti.peek().type() == Token::SYMBOL) {
            // Optional "-halo" 
            ti.readSymbol();
        }
        m_currentMaterial->d = ti.readNumber();
    } else if (cmd == "Tr") {
        // 1 - alpha on range [0,1]
        m_currentMaterial->Tr = ti.readNumber();
    } else if (cmd == "Ns") {
        // 1 - alpha on range [0,1]
        m_currentMaterial->Ns = ti.readNumber();
    } else if (cmd == "Ni") {
        // 1 - alpha on range [0,1]
        m_currentMaterial->Ni = ti.readNumber();
    } else if (cmd == "Ka") {
        m_currentMaterial->Ka.r = ti.readNumber();
        m_currentMaterial->Ka.g = ti.readNumber();
        m_currentMaterial->Ka.b = ti.readNumber();
    } else if (cmd == "Ka") {
        m_currentMaterial->Kd.r = ti.readNumber();
        m_currentMaterial->Kd.g = ti.readNumber();
        m_currentMaterial->Kd.b = ti.readNumber();
    } else if (cmd == "Ks") {
        m_currentMaterial->Ks.r = ti.readNumber();
        m_currentMaterial->Ks.g = ti.readNumber();
        m_currentMaterial->Ks.b = ti.readNumber();
    } else if (cmd == "Ke") {
        m_currentMaterial->Ke.r = ti.readNumber();
        m_currentMaterial->Ke.g = ti.readNumber();
        m_currentMaterial->Ke.b = ti.readNumber();
    } else if (cmd == "map_Kd") {
        m_currentMaterial->map_Kd = FilePath::concat(m_basePath, removeLeadingSlash(trimWhitespace(ti.readUntilNewlineAsString())));
    } else if (cmd == "map_Ks") {
        m_currentMaterial->map_Ks = FilePath::concat(m_basePath, removeLeadingSlash(trimWhitespace(ti.readUntilNewlineAsString())));
    } else if (cmd == "map_bump" || cmd == "bump") {
        Token t = ti.peek();
        if (t.type() == Token::SYMBOL && t.string() == "-") {
            // There are options coming
            ti.readSymbol("-");
            const std::string& opt = ti.readSymbol();
            if (opt == "mm") {
                // bias and gain
                m_currentMaterial->bumpBias = ti.readNumber();
                m_currentMaterial->bumpGain = ti.readNumber();
            }
        }
        m_currentMaterial->map_bump = FilePath::concat(m_basePath, removeLeadingSlash(trimWhitespace(ti.readUntilNewlineAsString())));
    }
}

} // namespace G3D
