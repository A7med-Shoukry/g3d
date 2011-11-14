/**
 \file ArticulatedViewer.h
 
 Viewer for files that can be loaded by ArticulatedModel

 \maintainer Morgan McGuire
 \author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 \created 2007-05-31
 \edited  2011-07-26
 */
#ifndef ArticulatedViewer_h
#define ArticulatedViewer_h

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class ArticulatedViewer : public Viewer {
private:

    ArticulatedModel::Ref	m_model;
    int			        m_numFaces;
    int                         m_numVertices;

    ArticulatedModel::Part*    m_selectedPart;

    ArticulatedModel::Mesh*    m_selectedMesh;

    /** In the Mesh's cpuIndexAray index array */
    int                         m_selectedTriangleIndex;

    float                       m_scale;
    Vector3                     m_offset;

    /** Saves the geometry for the first part to a flat file */
    void saveGeometry();

public:
    ArticulatedViewer();
    virtual void onInit(const std::string& filename);
    virtual bool onEvent(const GEvent& e, App* app);
    virtual void onGraphics(RenderDevice* rd, App* app, const Lighting::Ref& lighting);

};

#endif 
