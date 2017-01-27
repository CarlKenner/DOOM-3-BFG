#ifndef GZDOOM_GL_RIFTHMD_H_
#define GZDOOM_GL_RIFTHMD_H_

#ifdef HAVE_OCULUS_API
extern "C" {
#include "OVR_CAPI.h"
}
#endif

// Framebuffer texture for intermediate rendering of Oculus Rift image
class RiftHmd {
public:
	RiftHmd();
	~RiftHmd() {destroy();}
	void destroy(); // release all resources
	ovrResult init_tracking();
	ovrResult init_graphics();
	bool bindToSceneFrameBufferAndUpdate();
	void bindToSceneFrameBuffer();
	int getFBHandle() const {return sceneFrameBuffer;}

	void paintHudQuad(float hudScale, float pitchAngle, float yawRange /* degrees */);
	void paintCrosshairQuad(const ovrPosef& eyePose, const ovrPosef& otherEyePose, bool reducedHud);
	void paintWeaponQuad(const ovrPosef& eyePose, const ovrPosef& otherEyePose, float weaponDist, float weaponHeight);
	void paintBlendQuad();

	ovrPosef& setSceneEyeView(int eye, float zNear, float zFar);
	ovrResult commitFrame();
	ovrResult submitFrame(float metersPerSceneUnit);
	void recenter_pose();
	const ovrPosef& getCurrentEyePose() const {return currentEyePose;}
	ovrSizei RiftHmd::getViewSize();

private:
	ovrResult init_scene_texture();
	void createShaders();


	unsigned int sceneFrameBuffer;
	unsigned int depthBuffer;
	unsigned int frameIndex;

	unsigned int hudQuadShader;
	int hqsViewMatrixLoc;
	int hqsProjMatrixLoc;
	int hqsHudTextureLoc;
	unsigned int hudVbo;

// #ifdef HAVE_OCULUS_API
	ovrTextureSwapChain sceneTextureSet;
	// ovrTexture * mirrorTexture;
	ovrSession hmd;
	ovrVector3f hmdToEyeOffset[2];
	ovrLayerEyeFov sceneLayer;
	ovrPosef currentEyePose;
	ovrVector3f poseOrigin;
// #endif
};

extern RiftHmd* sharedRiftHmd;

#endif // GZDOOM_GL_RIFTHMD_H_
