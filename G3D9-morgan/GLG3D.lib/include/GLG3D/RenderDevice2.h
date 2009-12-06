#ifndef G3D_RenderDevice2_h
#define G3D_RenderDevice2_h

namespace G3D {

/** RenderDevice makes all underlying OpenGL state changes lazy.

 Until you actually invoke() a shader or call beginRawGL() it does not
 necessarily change the OpenGL state.  This allows it to optimize out
 redundant or unnecessary changes, especially those arising from
 push/pop.  It also allows RenderDevice to return the state from
 accessors without actually making OpenGL calls, which are slow
 (sometimes catastrophically slow, if using multiple GPUs).

 Example:

<pre>
ArgList args;
args["diffuse"]   = diffuseTex;  // A uniform
args["thickness"] = 3.4f;        // A uniform
args["vertex"]    = vertexRange; // A vertex attribute
args["normal"]    = normalRange; // A vertex attribute

rd->invoke(toonShader, Primitive::TRI_LIST, index, args);


</pre>


<pre>
GlobalArgs args;
globalArgs["diffuse"]   = diffuseTex;
globalArgs["thickness"] = 3.4f;

StreamArgs streamArgs;
streamArgs.primitive  = Primitive::TRI_LIST;
streamArgs.index = index;
streamArgs["vertex"] = vertexRange;
streamArgs["normal"] = normalRange;

rd->invoke(toonShader, globalArgs, streamArgs);
</pre>

 */
class RenderDevice2 {
public:

    enum {
        /** All vertices; whatever the size of the vertex  */
        ALL = -1,

        /** Non-instanced is not the same as instanced with a count of
            1; they lead to different OpenGL calls. */
        NON_INSTANCED = -1};
    
    /** Non-indexed geometry.

        Corresponds to glDrawArrays, glDrawArraysInstanced.  There is no G3D equivalent of
        glMultiDrawArrays.        
    */
    void invoke
    (const Shader::Ref&, 
     const ArgList&, 
     Primitive type, 
     int startVertex = 0, 
     int numVertices = ALL,
     int numInstances = NON_INSTANCED);
    
    /** Indexed geometry.

        Corresponds to glDrawElements, glDrawElementsInstanced.   There is no G3D equivalent of
        glMultiDrawElements, glMultiDrawElementsBaseVertex, or glDrawRangeElements.
    */
    void invoke
    (const Shader::Ref&,
     const ArgList&, 
     Primitive type, 
     VertexRange indexArray, 
     int numInstances = NON_INSTANCED);

    /** Image processing. */
    void invoke
    (const Shader::Ref&, 
     const ArgList&, 
     const Rect2D& rect, 
     int numInstances = NON_INSTANCED);

    /** Full-viewport rectangle. */
    void invoke
    (const Shader::Ref&, 
     const ArgList&, 
     int numInstances = NON_INSTANCED);

private:

    class State {
    public:
        ScissorOptions   scissor;
        StencilOptions   stencil;

        BlendOptions     blend;

        AlphaTest        alphaTest;
        float            alphaTestConstant;

        Rect2D           viewport;
 
        bool             depthWrite;
        DepthTest        depthTest;
        // todo: depth clamp
        // todo: depth range
        // todo: depth clear value (is there one?)

        float            lineWidth;
        float            pointSize;
        // TODO: perspective point/line options?

        bool             colorWrite;
        Color4           colorClear;
        // TODO: draw buffers
        // TODO: read buffer

        // TODO: cull face

        CFrame           objectToWorldMatrix;

        Camera           camera;

        /** Used to flip the coordinate system when rendering to
            texture by setting the g3d_FrameBufferMatrix. */
        bool             invertY;

        /** If NULL, render to the default framebuffer */
        FrameBuffer::Ref frameBuffer;
    };

public:

    /** 
     Run \a shader on vertex stream \a s with uniform arguments \a a.

     This is the primary way to render objects in G3D. It corresponds to the
     OpenGL calls glUseProgram (for the shader), glUniform and glBindTexture (for arguments), 
     and glDrawIndexedPrimitives (for issuing the vertex stream).
    */
   void invoke
   (Shader2::Ref&  shader,
    GlobalArgs&    a,
    StreamArgs&    s,
    int            numInstances = 1);

    /** Call to begin a block of raw OpenGL commands issued manually  
       (e.g., by calling gl___ functions yourself).  Doing so turns off
       RenderDevice's management of OpenGL until the corresponding endRawGL() call.
       There is significant overhead in entering such a block because RenderDevice
       must back up all OpenGL state.
       \sa endRawGL() */
    void beginRawGL();
    void endRawGL();

};

}

#endif
