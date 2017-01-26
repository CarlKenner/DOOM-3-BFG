#include "gl/scene/gl_hudtexture.h"
#include "gl/system/gl_system.h"
#include "gl/system/gl_cvars.h"
#include "gl/scene/gl_stereo3d.h"
#include "gl/renderer/gl_renderstate.h"
#include "gl/data/gl_vertexbuffer.h"
#include <cstring>

using namespace std;

HudTexture::HudTexture(int screenWidth, int screenHeight, float screenScale)
	// screenSizeScale to reduce texture size, so map lines could show up better
	: screenSizeScale(screenScale)
	, w( (int)(screenSizeScale*screenWidth) )
	, h( (int)(screenSizeScale*screenHeight) )
	, frameBuffer(0)
	, renderedTexture(0)
	, m_isBound(false)
{
	// Framebuffer
	glGenFramebuffers(1, &frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glGenTextures(1, &renderedTexture);
	glBindTexture(GL_TEXTURE_2D, renderedTexture);
	init(w, h);
	// clean up
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

bool HudTexture::checkScreenSize(int screenWidth, int screenHeight) {
	if ((w == (int)(screenSizeScale*screenWidth)) && (h == (int)(screenSizeScale*screenHeight)))
		return true; // no change
	return false;
}

void HudTexture::destroy() {
	glDeleteTextures(1, &renderedTexture);
	renderedTexture = 0;
	glDeleteFramebuffers(1, &frameBuffer);
	frameBuffer = 0;
}

void HudTexture::init(int width, int height) {
	// Framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	// Image texture
	glBindTexture(GL_TEXTURE_2D, renderedTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	// Poor filtering. Needed !
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// Don't let image wrap, especially in Rift mode
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderedTexture, 0);
	// clean up
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void HudTexture::bindToFrameBuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderedTexture, 0);
	glViewport(0, 0, w, h);
	m_isBound = true;
}

void HudTexture::bindRenderTexture() {
	glBindTexture(GL_TEXTURE_2D, renderedTexture);
}

void HudTexture::renderToScreen() {
	// Load that texture we just rendered
	gl_RenderState.EnableTexture(true);
	glBindTexture(GL_TEXTURE_2D, renderedTexture);
	// Very simple draw routine maps texture onto entire screen; no glOrtho or whatever!
	// gl_MatrixStack.Push(gl_RenderState.mProjectionMatrix);
	// gl_MatrixStack.Push(gl_RenderState.mModelMatrix);
	// gl_MatrixStack.Push(gl_RenderState.mViewMatrix);
	gl_RenderState.mProjectionMatrix.loadIdentity();
	gl_RenderState.mModelMatrix.loadIdentity();
	gl_RenderState.mViewMatrix.loadIdentity();

	gl_RenderState.ApplyMatrices();
	gl_RenderState.Apply();
	FFlatVertex *ptr = GLRenderer->mVBO->GetBuffer();
	ptr->Set(-1,  1, 0.5,  0, 1); ptr++;
	ptr->Set( 1,  1, 0.5,  1, 1); ptr++;
	ptr->Set(-1, -1, 0.5,  0, 0); ptr++;
	ptr->Set( 1, -1, 0.5,  1, 0); ptr++;
	GLRenderer->mVBO->RenderCurrent(ptr, GL_TRIANGLE_STRIP);

	glBindTexture(GL_TEXTURE_2D, 0);
	// gl_MatrixStack.Pop(gl_RenderState.mViewMatrix);
	// gl_MatrixStack.Pop(gl_RenderState.mModelMatrix);
	// gl_MatrixStack.Pop(gl_RenderState.mProjectionMatrix);
}

void HudTexture::unbind() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	m_isBound = false;
}


/// Static methods used for global singleton access

/* static */
HudTexture* HudTexture::hudTexture = nullptr;
HudTexture* HudTexture::crosshairTexture = nullptr;

EXTERN_CVAR(Float, vr_hud_scale);
EXTERN_CVAR(Int, vr_mode);

/* static */
void HudTexture::bindGlobalOffscreenBuffer()
{
	// Choose size based on stereo mode
	float hudScreenScale = 1.0;

	switch (vr_mode) {
	case Stereo3D::OCULUS_RIFT:
		hudScreenScale = 0.5 * vr_hud_scale;
		break;
	case Stereo3D::SIDE_BY_SIDE:
	case Stereo3D::SIDE_BY_SIDE_SQUISHED:
		hudScreenScale = 0.5;
		break;
	default:
		unbindGlobalOffscreenBuffer();
		return; // no offscreen hud for single image 3d modes
	}

	// Allocate, if necessary
	if (hudTexture)
		hudTexture->setScreenScale(hudScreenScale); // BEFORE checkScreenSize
	if ( (hudTexture == NULL) || (! hudTexture->checkScreenSize(SCREENWIDTH, SCREENHEIGHT) ) ) {
		if (hudTexture)
			delete(hudTexture);
		hudTexture = new HudTexture(SCREENWIDTH, SCREENHEIGHT, hudScreenScale);
		hudTexture->bindToFrameBuffer();
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	hudTexture->bindToFrameBuffer();
	glViewport(0, 0, hudTexture->getWidth(), hudTexture->getHeight());
	glScissor(0, 0, hudTexture->getWidth(), hudTexture->getHeight());
}

void HudTexture::bindAndClearGlobalOffscreenBuffer()
{
	bindGlobalOffscreenBuffer();
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
}


void HudTexture::unbindGlobalOffscreenBuffer() 
{
	if (hudTexture == NULL)
		return;
	if (! hudTexture->m_isBound)
		return;
	hudTexture->unbind();
}

/* static */
void HudTexture::displayAndClearGlobalOffscreenBuffer()
{
	float yScale = 1.0; // how much to stretch hud in vertical direction
	int hudOffsetX = 0;

	switch (vr_mode) {
	case Stereo3D::OCULUS_RIFT:
		yScale = 1.0;
		hudOffsetX = (int)(0.004*SCREENWIDTH/2); // kludge to set hud distance
		break;
	case Stereo3D::SIDE_BY_SIDE:
	case Stereo3D::SIDE_BY_SIDE_SQUISHED:
		yScale = 2.0;
		break;
	default:
		return; // no hud texture for single display modes
	}

	if (hudTexture == NULL)
		return;

	unbindGlobalOffscreenBuffer();

	// glEnable(GL_TEXTURE_2D);
	glDisable(GL_SCISSOR_TEST);
	// glEnable(GL_BLEND);

	// Compute viewport coordinates
	float h = hudTexture->getHeight() * yScale;
	float w = hudTexture->getWidth();
	float y = (SCREENHEIGHT-h)*0.5; // offset to move cross hair up to correct spot

	glScissor(0, 0, SCREENWIDTH, SCREENHEIGHT); // Not infinity, but not as close as the weapon.

	// Left side
	float x = (SCREENWIDTH/2-w)*0.5;
	glViewport(x+hudOffsetX, y, w, h); // Not infinity, but not as close as the weapon.
	glScissor(x+hudOffsetX, y, w, h); // Not infinity, but not as close as the weapon.
	hudTexture->renderToScreen();

	// Right side
	x += SCREENWIDTH/2;
	glViewport(x-hudOffsetX, y, w, h);
	glScissor(x-hudOffsetX, y, w, h);
	hudTexture->renderToScreen();

	// Clear hud for next pass
	bindGlobalOffscreenBuffer();
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	unbindGlobalOffscreenBuffer();
	glViewport(0, 0, SCREENWIDTH, SCREENHEIGHT);
	glScissor(0, 0, SCREENWIDTH, SCREENHEIGHT);
}


