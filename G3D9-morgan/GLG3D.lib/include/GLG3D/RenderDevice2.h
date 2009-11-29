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
