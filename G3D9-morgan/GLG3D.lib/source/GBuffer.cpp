// Compute the passes:

GBuffer::GBuffer(int w, int h, const Settings& s) : 
    m_width(w), m_height(h), m_settings(s),
    m_texturesPerPass(GLCaps::something())
{
    generateShaders();
    allocate();
}


void GBuffer::generateShaders() {
    
    // May be zero
    const int numPasses = 
        iCeil(m_settings.bufferArray.size() / float(m_texturesPerPass));

    m_passArray.resize(numPasses);

    // buffer index
    int b = 0;

    for (int p = 0; p < numPasses; ++p) {
        Pass& pass = m_passArray[p];

        int bufferThisPass = 
            min(m_buffersPerPass, 
                m_settings.bufferArray.size() - b + 1);

        pass.bufferTypeArray.resize(buffersThisPass);

        bool faceNormals = false;

        for (int i = 0; i < buffersThisPass; ++i) {
            pass.bufferTypeArray[i] = bufferArray[b];

            faceNormals = faceNormals || 
                (pass.bufferTypeArray[i] == FACE_NORMALS);

            ++b;
        }

        // Generate the actual shader
        std::string vrt;
        std::string geo;
        std::string pix;

        if (faceNormals) {
            geo = 
                STR(
#version 120 
#extension GL_EXT_geometry_shader4 : enable
varying in vec3 normalIn[3];
varying out vec2 texCoordIn[3];

varying out vec3 normal;
varying out vec3 faceNormal;
varying out vec2 texCoord;

// TODO: pass all other needed values through

uniform mat4 MVP;

void main() {
    vec3 commonFaceNormal =
        normalize(cross(gl_PositionIn[1].xyz - gl_PositionIn[0].xyz, 
                        gl_PositionIn[2].xyz - gl_PositionIn[0].xyz)) * 0.5 + vec3(0.5);

    // Simple surface expansion and per-face coloring example
    for (int i = 0; i < 3; ++i){
        gl_Position = MVP * gl_PositionIn[i];
        normal      = normalIn[i];
        texCoord    = texCoordIn[i];
        faceNormal  = commonFaceNormal;
        EmitVertex();
    }
    EndPrimitive();
});
        }

        pass.shader = Shader::fromStrings(vrt, geo, pix);
        pass.frameBuffer = FrameBuffer::create();
        
    }
}


void GBuffer::allocate() {
    int i = 0;
    for (int p = 0; p < m_passArray.size(); ++p) {
        Pass& pass = m_passArray[p];

        // Attach to the framebuffer
    }
}


void GBuffer::update
(RenderDevice*                    rd, 
 const Camera&                    camera,
 const Array<Surface::Ref>&       surfaceArray) {
    
    m_camera = camera;
    
    for (int p = 0; p < m_passArray.size(); ++p) {
        const Pass& pass = m_passArray[p];
        rd->pushState(pass.frameBuffer);
        {
            rd->setProjectionAndCameraMatrix(camera);

            const bool firstPass = (pass == 0);
            rd->setDepthWrite(firstPass);
            rd->clear(true, firstPass, false);

            for (int s = 0; s < surfaceArray.size(); ++s) {
                const SuperSurface::Ref& surface = 
                    superSurface[s].downcast<SuperSurface>();

                if (surface.notNull()) {
                    rd->setObjectToWorldMatrix(surface->coordinateFrame());
                    surface->sendGeometry(rd);
                }
            }
        }
        rd->popState();
    }
}

