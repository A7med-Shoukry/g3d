#include "ParseOBJ.h"


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
    }

    ti.popSettings();
}


void ParseOBJ::processCommand(TextInput& ti, const std::string& cmd) {

    if (cmd == "mtllib") {

        // Specify material library 
        std::string mtlFilename = trimWhitespace(ti.readUntilNewlineAsString());
        mtlFile = FilePath::concat(m_basePath, mtlFilename);

        m_currentMaterialTable.parse(TextInput(mtlFile));

    } else if (cmd == "g") {

        // New group
        currentTriListRawName = trimWhitespace(ti.readUntilNewlineAsString());
        if (! groupTable.containsKey(currentTriListRawName)) {
            currentTriList = new TriListSpec();
            currentTriList->name = currentTriListRawName;
            groupTable.set(currentTriListRawName, currentTriList);
        } else {
            currentTriList = groupTable[currentTriListRawName];
        }

    } else if (cmd == "usemtl") {

        // If the current tri list is empty, assign a material to it.  Otherwise break
        // the trilist here and start a new one.
        if (currentTriList) {
            const std::string& materialName = trimWhitespace(ti.readUntilNewlineAsString());
            if (currentTriList->cpuIndex.size() != 0) {
                const std::string& triListName = currentTriListRawName + "_" + materialName;
                debugAssertM(groupTable.containsKey(currentTriListRawName),
                                "Hit a usemtl block when currentTriList != NULL but the tri list had no name.");

                if (groupTable[currentTriListRawName]->materialName == materialName) {
                    // Switch back to the base trilist, which uses this material
                    currentTriList = groupTable[currentTriListRawName];
                } else {
                    // Find or create the trilist that uses this material

                    if (! groupTable.containsKey(triListName)) {
                        currentTriList = new TriListSpec();
                        currentTriList->name = triListName;            
                        groupTable.set(triListName, currentTriList);
                    } else {
                        currentTriList = groupTable[triListName];
                    }
                }
            }

            currentTriList->materialName = materialName;
        }
    } else if (cmd == "v") {
        rawVertex.append(readVertex(ti, preprocess.xform));
    } else if (cmd == "vt") {
        // Texcoord
        Vector2& t = rawTexCoord.next();
        t.x = ti.readNumber();
        t.y = 1.0f - ti.readNumber();
    } else if (cmd == "vn") {
        // Normal
        rawNormal.append(readNormal(ti, normalXform));
    } else if ((cmd == "f") && currentTriList) {
        // Face

        // Read each vertex
        while (ti.hasMore() && (ti.peek().type() != Token::NEWLINE)) {

            // Read one 3-part index
            int v = ti.readNumber();
            if (v < 0) {
                v = rawVertex.size() + v + 1;
            }

            int n = 0;
            int t = 0;

            if (ti.peek().type() == Token::SYMBOL) {
                // Optional texcoord and normal
                ti.readSymbol("/");
                if (ti.peek().type() == Token::NUMBER) {
                    t = ti.readNumber();
                    if (t < 0) {
                        t = rawTexCoord.size() + t + 1;
                    }
                }
                if (ti.peek().type() == Token::SYMBOL) {
                    ti.readSymbol("/");
                    if (ti.peek().type() == Token::NUMBER) {
                        n = ti.readNumber();
                        if (n < 0) {
                            n = rawNormal.size() + n + 1;
                        }
                    }
                }
            }

            // Switch to zero-based indexing 
            --v; --n; --t;

            faceTempIndex.append(v, t, n);
        }

        alwaysAssertM(faceTempIndex.size() >= 3*3, "Face with fewer than three vertices in model.");
        numTris += (faceTempIndex.size()/3) - 2;
        // The faceTempIndex is now a triangle fan.  Convert it to a triangle list and use unique vertices
        for (int i = 2; i < faceTempIndex.size()/3; ++i) {
            // Always start with vertex 0
            cookVertex.append(faceTempIndex[0]);
            cookTexCoord.append(faceTempIndex[1]);
            cookNormal.append(faceTempIndex[2]);

            // The vertex just before the one we're adding
            int j = (i - 1) * 3;
            cookVertex.append(faceTempIndex[j]);
            cookTexCoord.append(faceTempIndex[j+1]);
            cookNormal.append(faceTempIndex[j+2]);

            // The vertex we're adding
            j = i * 3;
            cookVertex.append(faceTempIndex[j]);
            cookTexCoord.append(faceTempIndex[j+1]);
            cookNormal.append(faceTempIndex[j+2]);

            // Update the index array to contain the three vertices we just added
            currentTriList->cpuIndex.append(cookVertex.size() - 3, cookVertex.size() - 2, cookVertex.size() - 1);
        } 

        faceTempIndex.fastClear();

    }
