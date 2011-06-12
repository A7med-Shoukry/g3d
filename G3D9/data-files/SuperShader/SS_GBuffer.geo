// -*- c++ -*-
/** \file SS_GBuffer.geo 
    \author Morgan McGuire, http://graphics.cs.williams.edu */
// This shader expects a prefix attached at runtime in SuperShader.cpp

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

varying in  vec3 wsPositionIn[3];
varying in  vec2 texCoordIn[3];
varying in  vec3 tan_ZIn[3];

varying out vec3 wsFaceNormal;
varying out vec3 wsPosition;
varying out vec2 texCoord;
varying out vec3 tan_Z; 

#ifdef NORMALBUMPMAP  
#   if PARALLAXSTEPS > 0
        /** Vector to the eye in tangent space (needed for parallax) */
        varying in vec3 _tsEIn[3];
        varying out vec3 _tsE;
#   endif

	/** Tangent space to world space.
    Note that this will be linearly interpolated across the polygon.

	NVIDIA drivers do not properly interpolate mat4, so we must pass
	the columns along a separate vectors. */
	varying in vec3 tan_XIn[3], tan_YIn[3];
	varying out vec3 tan_X, tan_Y;
#endif

void main() {
    // Unit face normal in world space
    vec3 N = normalize(cross(wsPositionIn[1] - wsPositionIn[0], wsPositionIn[2] - wsPositionIn[0]));

    // For each vertex
    for (int v = 0; v < 3; ++v) {
        gl_Position     = gl_PositionIn[v];
        gl_PrimitiveID  = gl_PrimitiveIDIn;

        texCoord        = texCoordIn[v];
#       ifdef NORMALBUMPMAP  
#           if PARALLAXSTEPS > 0
                _tsE    = _tsEIn[v];
#           endif
            tan_X       = tanXIn[v];
            tan_Y       = tanYIn[v];
#       endif
        tanZ            = tan_ZIn[v];

        wsPosition      = wsPositionIn[v];
        wsFaceNormal    = N;

        EmitVertex();
    }
}
