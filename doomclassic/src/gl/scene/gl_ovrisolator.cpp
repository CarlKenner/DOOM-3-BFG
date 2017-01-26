#include "OVR.h"
#include <Windows.h>
#include "OVR_CAPI_GL.h"

// Compile unit to firewall <Windows.h>-needing crap in OVR_CAPI_GL.h from the rest of the application

class OvrSdkRenderer {
public:
	OvrSdkRenderer();
	virtual ~OvrSdkRenderer();
	void init(int texWidth, int texHeight, int textureId);
	void predraw();

private:
	ovrHmd hmd;
	int frameIndex;
	ovrEyeRenderDesc eyeRenderDescs[2];
};


OvrSdkRenderer::OvrSdkRenderer()
{
	ovr_Initialize();
	hmd = ovrHmd_Create(0);
	if (hmd) {
		ovrHmd_ConfigureTracking(hmd,
			ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position, // supported
			ovrTrackingCap_Orientation); // required

		// Set low persistence mode
		int hmdCaps = ovrHmd_GetEnabledCaps(hmd);
		ovrHmd_SetEnabledCaps(hmd, hmdCaps | ovrHmdCap_LowPersistence);
	}
}


OvrSdkRenderer::~OvrSdkRenderer() 
{
	ovrHmd_Destroy(hmd);
	ovr_Shutdown();
}


void OvrSdkRenderer::init(int texWidth, int texHeight, int textureId) 
{
	ovrRenderAPIConfig cfg;
	// int configResult = ovrHmd_ConfigureRendering(hmd, &cfg, distortionCaps, hmd->DefaultEyeFov, eyeRenderDescs);

	// ovrHmd_AttachToWindow(); // TODO

	ovrSizei renderTargetSize = {texWidth, texHeight};
	ovrRecti leftViewport = {0, 0, texWidth/2, texHeight};
	ovrRecti rightViewport = {texWidth/2, 0, texWidth/2, texHeight};

	ovrGLTexture ovrEyeTexture[2];
	ovrEyeTexture[0].OGL.Header.API = ovrRenderAPI_OpenGL;
	ovrEyeTexture[0].OGL.Header.TextureSize = renderTargetSize;
	ovrEyeTexture[0].OGL.Header.RenderViewport = leftViewport;
	ovrEyeTexture[0].OGL.TexId = textureId;

	ovrEyeTexture[1] = ovrEyeTexture[0];
	ovrEyeTexture[1].OGL.Header.RenderViewport = rightViewport;

}

void OvrSdkRenderer::predraw() 
{
	ovrPosef eyePoses[2]; // to hold output of ovrHmd_GetEyePoses, below
	// ovrHmd_GetEyePoses(hmd, frameIndex, eyeOffsets, eyePoses, NULL);
}
