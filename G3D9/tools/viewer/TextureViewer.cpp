/**
 @file TextureViewer.cpp
 
 Viewer for image files
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#include "TextureViewer.h"

static bool allCubeMapFacesExist(const std::string& base, const std::string& ext, std::string& wildcardBase) {

	CubeMapConvention matchedConv = CubeMapConvention::G3D;
	bool foundMatch = false;

	// Check of filename ends in one of the convensions
	for (int convIndex = 0; convIndex < CubeMapConvention::COUNT; ++convIndex) {
		const Texture::CubeMapInfo& info = Texture::cubeMapInfo(static_cast<CubeMapConvention>(convIndex));

		for (int faceIndex = 0; faceIndex < 6; ++faceIndex) {
			if (endsWith(base, info.face[faceIndex].suffix)) {
				foundMatch = true;
				matchedConv = static_cast<CubeMapConvention>(convIndex);
				wildcardBase = base.substr(0, base.length() - info.face[faceIndex].suffix.length());
				break;
			}
		}
	}

	// Texture isn't the start of a cube map set at all
	if (! foundMatch) {
		return false;
	}

    bool success = true;
	const Texture::CubeMapInfo& info = Texture::cubeMapInfo(matchedConv);


	// See if all faces exist
	for (int faceIndex = 0; faceIndex < 6; ++faceIndex) {
		if (! FileSystem::exists(wildcardBase + info.face[faceIndex].suffix + "." + ext)) {
			success = false;
			break;
		}
    }

	return success;
}


TextureViewer::TextureViewer() :
    m_texture(NULL),
    m_width(0),
    m_height(0), 
	m_isSky(false) {

}


void TextureViewer::onInit(const std::string& filename) {

	// Determine if texture is part of a cubemap set
	const std::string& path = FilePath::parent(filename);
	const std::string& base = FilePath::base(filename);
    const std::string& ext = FilePath::ext(filename);

	std::string wildcardBase;
	if (allCubeMapFacesExist(FilePath::concat(path, base), ext, wildcardBase)) {
		m_isSky = true;

		m_texture = Texture::fromFile(wildcardBase + "*." + ext, ImageFormat::AUTO(), Texture::DIM_CUBE_MAP_NPOT, Texture::Settings::cubeMap());
	} else {
		m_texture = Texture::fromFile( filename, ImageFormat::AUTO(), Texture::DIM_2D_NPOT, Texture::Settings::video() );
		m_height = m_texture->height();
		m_width = m_texture->width();
	}
}


void TextureViewer::onGraphics(RenderDevice* rd, App* app, const LightingRef& lighting) {
	if (m_isSky) {

		rd->disableLighting();
        Draw::skyBox(rd, m_texture);

	} else {

		screenPrintf("Width: %d",  m_width);
		screenPrintf("Height: %d",  m_height);

		rd->push2D();
			
			// Creates a rectangle the size of the window.
			float windowHeight = rd->viewport().height();
			float windowWidth = rd->viewport().width();
			
			Rect2D rect;
			if((windowWidth > m_width) && (windowHeight > m_height)){
				// creates a rectangle the size of the texture
				// centered in the window
				rect = Rect2D::xywh(windowWidth/2 - m_width/2,
									windowHeight/2 - m_height/2,
									m_width,
									m_height);
			} else {
				// Creates a rectangle the size of the texture
				// in the top left corner of the window, if the window
				// is smaller than the image currently
				rect = m_texture->rect2DBounds();
			}

			rd->setTexture(0, m_texture);
			Draw::rect2D(rect, rd);
		rd->pop2D();

	}
}
