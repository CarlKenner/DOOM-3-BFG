#include "gl/scene/gl_offscreenbuffermanager.h"
#include "gl/scene/gl_hudtexture.h"
#include "gl/system/gl_system.h"
#include "v_video.h"

OffscreenBufferManager::OffscreenBufferManager() 
{
	HudTexture::bindGlobalOffscreenBuffer();
}

/* virtual */
OffscreenBufferManager::~OffscreenBufferManager() 
{
	HudTexture::unbindGlobalOffscreenBuffer();
	HudTexture::displayAndClearGlobalOffscreenBuffer();
}

/* static */
void OffscreenBufferManager::bind() {
	// HudTexture::unbindGlobalOffscreenBuffer();
	// glClearColor(0.5, 0, 0, 0);
	// glViewport(0, 0, SCREENWIDTH, SCREENHEIGHT);
	HudTexture::bindGlobalOffscreenBuffer();
}


