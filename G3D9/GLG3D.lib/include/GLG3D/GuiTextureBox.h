/**
 @file GLG3D/GuiTextureBox.h

 @created 2009-09-11
 @edited  2009-09-11

 G3D Library http://g3d.sf.net
 Copyright 2000-2011, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
*/
#ifndef G3D_GuiTextureBox_h
#define G3D_GuiTextureBox_h

#include "G3D/platform.h"
#include "G3D/Vector2.h"
#include "GLG3D/GuiContainer.h"
#include "GLG3D/Texture.h"
#include "GLG3D/GuiText.h"

namespace G3D {

class GuiPane;
class GuiButton;
class GuiTextureBoxInspector;

class GuiTextureBox : public GuiContainer {
public:
    friend class GuiTextureBoxInspector;

protected:

    /** Padding around the image */
    enum {BORDER = 1};

    Texture::Ref             m_texture;

    WeakReferenceCountedPointer<GuiTextureBoxInspector>     m_inspector;

    Texture::Visualization   m_settings;

    /** Bounds for mouse clicks and scissor region, updated by every render. */
    Rect2D                   m_clipBounds;
    
    bool                     m_showInfo;

    /** Cached formatting of m_lastSize */
    mutable GuiText          m_lastSizeCaption;
    mutable Vector2int16     m_lastSize;
    mutable std::string      m_lastFormat;

    GuiText                  m_drawerCollapseCaption;
    GuiText                  m_drawerExpandCaption;
    GuiButton*               m_drawerButton;
    GuiPane*                 m_drawerPane;
    bool                     m_drawerOpen;

    mutable GuiButton*       m_saveButton;
    mutable GuiButton*       m_inspectorButton;

    Shader::Ref              m_shader;

    float                    m_zoom;
    Vector2                  m_offset;

    /** True when dragging the image */
    bool                     m_dragging;
    Vector2                  m_dragStart;
    Vector2                  m_offsetAtDragStart;

    /** Readback texel */
    mutable Color4           m_texel;
    /** Readback position */
    mutable Vector2int16     m_readbackXY;

    bool                     m_embeddedMode;

    bool                     m_showFormat;

    static WeakReferenceCountedPointer<Shader> g_cachedShader;

    /** Returns the bounds of the canvas (display) region for this GuiTextBox */
    Rect2D canvasRect() const;

    /** Returns the bounds of the canvas (display) region for a GuiTextBox of size \a rect*/
    Rect2D canvasRect(const Rect2D& rect) const;

    void drawTexture(RenderDevice* rd, const Rect2D& r) const;

public:

    /** In most cases, you'll want to call GuiPane::addTextureBox instead.

      \param embeddedMode When set to true, hides the controls that duplicate inspector functionality.
     */
    GuiTextureBox
    (GuiContainer*       parent,
     const GuiText&      caption,
     const Texture::Ref& t = NULL,
     bool                embeddedMode = false);

    virtual ~GuiTextureBox();

    /** Starts the inspector window.  Invoked by the inspector button. */
    void showInspector();

    /** Zoom factor for the texture display.  Greater than 1 = zoomed in. */
    inline float viewZoom() const {
        return m_zoom;
    }
    
    void setViewZoom(float z);

    /** Offset of the texture from the centered position. Positive = right and down. */
    inline const Vector2& viewOffset() const {
        return m_offset;
    }

    void setShowFormat(bool f) {
        m_showFormat = f;
    }

    bool showFormat() const {
        return m_showFormat;
    }

    void zoomIn();
    void zoomOut();

    /** Brings up the modal save dialog */
    void save();

    void setViewOffset(const Vector2& x);

    /** Change the scale to 1:1 pixel */
    void zoomTo1();

    /** Center the image and scale it to fill the viewport */
    void zoomToFit();

    void setTexture(const Texture::Ref& t);
    void setSettings(const Texture::Visualization& s);

    inline const Texture::Ref& texture() const {
        return m_texture;
    }

    inline const Texture::Visualization& settings() const {
        return m_settings;
    }

    /** Controls the display of (x,y)=rgba when the mouse is over the box.
        Defaults to true.  Note that displaying these values can significantly
        impact performance because it must read back from the GPU to the CPU.*/
    inline void setShowInfo(bool b) {
        m_showInfo = b;
    }

    inline bool showInfo() const {
        return m_showInfo;
    }

    /** Sizes the control so that exactly \a dims of viewing space is available. 
        Useful for ensuring that textures are viewed at 1:1.*/
    void setSizeFromInterior(const Vector2& dims);

    virtual void render(RenderDevice* rd, const GuiTheme::Ref& theme) const;
    virtual void setRect(const Rect2D& rect);
    virtual void findControlUnderMouse(Vector2 mouse, GuiControl*& control) const;

    virtual bool onEvent(const GEvent& event);

    /** Invoked by the drawer button. Do not call directly. */
    void toggleDrawer();
};

} // namespace

#endif
