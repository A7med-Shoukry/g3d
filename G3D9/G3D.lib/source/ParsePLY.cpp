/**
 \file G3D/source/ParsePLY.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-23
 \edited  2011-07-23
 
 Copyright 2000-2011, Morgan McGuire.
 All rights reserved.
*/
#include "G3D/ParsePLY.h"
#include "G3D/BinaryInput.h"
#include "G3D/FileSystem.h"
#include "G3D/stringutils.h"
#include "G3D/ParseError.h"

namespace G3D {
    
ParsePLY::ParsePLY() : vertexData(NULL), faceArray(NULL) {}


void ParsePLY::clear() {
    delete[] vertexData;
    vertexData = NULL;
    delete[] faceArray;
    faceArray = NULL;
    numVertices = numFaces = 0;
}


ParsePLY::~ParsePLY() {
    clear();
}


void ParsePLY::parse(BinaryInput& bi) {
    const G3DEndian oldEndian = bi.endian();

    clear();
    readHeader(bi);

    faceArray = new Face[numFaces];
    vertexData = new float[numVertices * vertexProperty.size()];

    readVertexList(bi);
    readFaceList(bi);

    bi.setEndian(oldEndian);
}


ParsePLY::DataType ParsePLY::parseDataType(const char* t) {
    static const char* names[] = {"char", "uchar", "short", "ushort", "int", "uint", "float", "double", "list", NULL};

    for (int i = 0; names[i] != NULL; ++i) {
        if (strcmp(t, names[i]) == 0) {
            return DataType(i);
        }
    }

    throw std::string("Illegal type specifier: ") + t;
    return none_type;
}


void ParsePLY::parseProperty(const std::string& s, Property& prop) {
    // Examples:
    //
    // property float x
    // property list uchar int vertex_index

    char temp[100], name[100];

    sscanf(s.c_str(), "%*s %100s", temp);
    prop.type = parseDataType(temp);

    if (prop.type == list_type) {
        char temp2[100];
        // Read the index and element types
        sscanf(s.c_str(), "%*s %*s %100s %100s %100name", temp, temp2, name);
        prop.listLengthType = parseDataType(temp);
        prop.listElementType = parseDataType(temp2);
    } else {
        sscanf(s.c_str(), "%*s %*s  %100name", name);
    }

    prop.name = name;
}


size_t ParsePLY::byteSize(DataType d) {
    const size_t sz[] = {1, 1, 2, 2, 4, 4, 4, 8};
    alwaysAssertM((int)d >= 0 && (int)d < 8, "Illegal data type");
    return sz[d];
}


void ParsePLY::readHeader(BinaryInput& bi) {
    const std::string& hdr = bi.readStringNewline();
    if (hdr != "ply") {
        throw ParseError(bi.getFilename(), bi.getPosition(), format("Bad PLY header: \"%s\"", hdr.c_str()));
    }

    const std::string& fmt = bi.readStringNewline();
    
    if (fmt == "format binary_little_endian 1.0") {
        // Default format, nothing to do
        bi.setEndian(G3D_LITTLE_ENDIAN);
    } else if (fmt == "format binary_big_endian 1.0") {
        // Flip the endian
        bi.setEndian(G3D_BIG_ENDIAN);
    } else if (fmt == "format ascii 1.0") {
        throw ParseError(bi.getFilename(), bi.getPosition(), "ASCII PLY format is not supported in this release.");
    } else {
        throw ParseError(bi.getFilename(), bi.getPosition(), "Unsupported PLY format: " + fmt);
    }

    // Set to true when these fields are read
    bool readVertex = false;
    bool readFace = false;

    std::string s =  bi.readStringNewline();
    while (s != "end_header") {
        if (beginsWith(s, "comment ")) {

            // Ignore this line
            s = bi.readStringNewline();

        } if (beginsWith(s, "element vertex ")) {
            if (readVertex) {
                throw std::string("Already defined vertex.");
            }

            // Read the properties
            sscanf(s.c_str(), "%*s %d", &numVertices);

            s = bi.readStringNewline();
            while (beginsWith(s, "property ")) {
                parseProperty(s, vertexProperty.next());
                s = bi.readStringNewline();
            }

            readVertex = true;

        } else if (beginsWith(s, "element face ")) {
            if (! readVertex) {
                throw std::string("This implementation only supports faces following vertices.");
            }

            if (readFace) {
                throw std::string("Already defined faces.");
            }

            // Read the properties
            sscanf(s.c_str(), "%*s %d", &numFaces);

            s = bi.readStringNewline();
            while (beginsWith(s, "property ")) {
                parseProperty(s, vertexProperty.next());
                s = bi.readStringNewline();
            }

            readFace = true;

        } else {
            throw std::string("Unsupported PLY2 header command: ") + s;
        }
    }
}


template<class T>
static T readAs(ParsePLY::DataType type, BinaryInput& bi) {
    switch (type) {
    case ParsePLY::char_type:
        return (T)bi.readInt8();

    case ParsePLY::uchar_type:
        return (T)bi.readUInt8();

    case ParsePLY::short_type:
        return (T)bi.readInt16();

    case ParsePLY::ushort_type:
        return (T)bi.readUInt16();

    case ParsePLY::int_type:
        return (T)bi.readInt32();

    case ParsePLY::uint_type:
        return (T)bi.readUInt32();

    case ParsePLY::float_type:
        return (T)bi.readFloat32();

    case ParsePLY::double_type:
        return (T)bi.readFloat64();

    case ParsePLY::list_type:
        throw std::string("Tried to read a list as a value type");
        return (T)0;

    case ParsePLY::none_type:
        throw std::string("Tried to read an undefined type as a value type");
        return (T)0;
    }
    return (T)0;
}


float ParsePLY::readAsFloat(const Property& prop, BinaryInput& bi) {
    switch (prop.type) {
    case char_type:
    case uchar_type:
    case short_type:
    case ushort_type:
    case int_type:
    case uint_type:
    case float_type:
    case double_type:
        return readAs<float>(prop.type, bi);

    case list_type:
        {
            // Consume the list values
            const int n = readAs<int>(prop.listLengthType, bi);
            for (int i = 0; i < n; ++i) {
                readAs<float>(prop.listElementType, bi);
            }
        }
        return 0.0f;

    case none_type:
        throw std::string("Tried to read an undefined property");
        return 0.0f;
    };

    return 0.0f;
}


void ParsePLY::readVertexList(BinaryInput& bi) {
    alwaysAssertM(false, "TODO");

}


void ParsePLY::readFaceList(BinaryInput& bi) {
    alwaysAssertM(false, "TODO");

}

} // G3D

