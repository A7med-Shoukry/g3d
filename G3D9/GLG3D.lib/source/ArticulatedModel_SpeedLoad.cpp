#include "G3D/serialize.h"
#include "GLG3D/ArticulatedModel.h"

namespace G3D {

ArticulatedModel::Ref ArticulatedModel::speedCreate(BinaryInput& b) {
    ArticulatedModel::Ref m = new ArticulatedModel();
    m->speedDeserialize(b);
    return m;
}


void ArticulatedModel::speedDeserialize(BinaryInput& b) {
    SpeedLoad::readHeader(b, "ArticulatedModel");

    name = b.readString32();

    // Read all of the materials into a table
    Table<SpeedLoadIdentifier, Material::Ref> materialTable;
    const int numMaterials = b.readInt32();
    for (int m = 0; m < numMaterials; ++m) {
        SpeedLoadIdentifier sli;
        Material::Ref material = Material::speedCreate(sli, b);
        materialTable.set(sli, material);
    }

    partArray.clear();
    partArray.resize(b.readInt32());
    for (int p = 0; p < partArray.size(); ++p) {
        partArray[p].speedDeserialize(b, materialTable);
    }

    m_settings.deserialize(b);
}


void ArticulatedModel::speedSerialize(BinaryOutput& b) const {
    SpeedLoad::writeHeader(b, "ArticulatedModel");
    b.writeString32(name);

    Table<Material::Ref, SpeedLoadIdentifier> speedLoadIdentifierTable;
    Set<SpeedLoadIdentifier> usedSpeedLoadIdentifierSet;
    
    const int64 numMaterialsPosition = b.position();
    int numMaterials = 0;
    b.writeInt32(numMaterials); // Reserve space for the number of materials

    // Walk the tree, finding all materials in all triLists and writing them out
    for (int p = 0; p < partArray.size(); ++p) {
        const Part& part = partArray[p];
        for (int t = 0; t < part.triList.size(); ++t) {
            const Part::TriList::Ref& triList = part.triList[t];

            bool created = false;
            SpeedLoadIdentifier& sli = speedLoadIdentifierTable.getCreate(triList->material, created);

            // Serialize each material once. Recognize duplicates by both SpeedLoadIdentifier and by
            // pointer.
            if (! created) {
                // This exact pointer doesn't exist.  Serialize and see if it is in fact unique
                const int64 beforePosition = b.position();
                triList->material->speedSerialize(sli, b);

                if (usedSpeedLoadIdentifierSet.contains(sli)) {
                    // This was previously written; back out the serialization
                    b.setPosition(beforePosition);
                } else {
                    // This is indeed a new material, so leave the data that we just wrote
                    ++numMaterials;
                }
            }
        }
    }
    
    const int64 endOfMaterialsPosition = b.position();
    b.setPosition(numMaterialsPosition);
    b.writeInt32(numMaterials);
    b.setPosition(endOfMaterialsPosition);

    b.writeInt32(partArray.size());
    for (int p = 0; p < partArray.size(); ++p) {
        partArray[p].speedSerialize(b, speedLoadIdentifierTable);
    }

    b.writeInt32(m_numTriangles);

    m_settings.serialize(b);
}

/////////////////////////////////////////////////////////////////////////////////

void ArticulatedModel::Part::speedSerialize(BinaryOutput& b, const Table<Material::Ref, SpeedLoadIdentifier>& speedLoadIdentifierTable) const {
    SpeedLoad::writeHeader(b, "ArticulatedModel::Part");
    serialize(name, b);
    cframe.serialize(b);

    // Don't write the VertexRanges, just dump from the CPU data
    serialize(geometry.vertexArray, b);
    serialize(geometry.normalArray, b);
    serialize(texCoordArray, b);
    serialize(packedTangentArray, b);

    b.writeInt32(triList.size());
    for (int t = 0; t < triList.size(); ++t) {
        triList[t]->speedSerialize(b, speedLoadIdentifierTable);
    }

    serialize(subPartArray, b);
    serialize(parent, b);
    serialize(indexArray, b);
}

void ArticulatedModel::Part::speedDeserialize(BinaryInput& b, const Table<SpeedLoadIdentifier, Material::Ref>& materialTable) {
    SpeedLoad::readHeader(b, "ArticulatedModel::Part");

    deserialize(name, b);

    cframe.deserialize(b);

    // Don't read the VertexRanges explicitly; load the CPU data
    deserialize(geometry.vertexArray, b);
    deserialize(geometry.normalArray, b);
    deserialize(texCoordArray, b);
    deserialize(packedTangentArray, b);

    triList.resize(b.readInt32());
    for (int t = 0; t < triList.size(); ++t) {
        triList[t] = new TriList();
        triList[t]->speedDeserialize(b, materialTable);
    }

    deserialize(subPartArray, b);
    deserialize(parent, b);
    deserialize(indexArray, b);

    updateVAR();
}

/////////////////////////////////////////////////////////////////////

void ArticulatedModel::Part::TriList::speedSerialize(BinaryOutput& b, const Table<Material::Ref, SpeedLoadIdentifier>& speedLoadIdentifierTable) const {
    SpeedLoad::writeHeader(b, "ArticulatedModel::Part::TriList");

    // Fields inherited from GPUGeom
    primitive.serialize(b);

    // Don't write the VertexArrays; they will be shared with the Part automatically
    serialize(twoSided, b);

    // The material itself was saved up front
    speedLoadIdentifierTable[material].serialize(b);

    boxBounds.serialize(b);
    sphereBounds.serialize(b);
    serialize(indexArray, b);

}


void ArticulatedModel::Part::TriList::speedDeserialize(BinaryInput& b, const Table<SpeedLoadIdentifier, Material::Ref>& materialTable) {
    SpeedLoad::readHeader(b, "ArticulatedModel::Part::TriList");

    primitive.deserialize(b);
    deserialize(twoSided, b);
    SpeedLoadIdentifier sli(b);
    material = materialTable[sli];
    boxBounds.deserialize(b);
    sphereBounds.deserialize(b);
    deserialize(indexArray, b);
}

} // G3D
