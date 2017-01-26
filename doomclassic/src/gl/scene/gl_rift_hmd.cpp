#define NOMINMAX
#include "gl/scene/gl_rift_hmd.h"
#include "gl/scene/gl_hudtexture.h"
#include "gl/system/gl_system.h"
#include "gl/system/gl_cvars.h"
#include "gl/renderer/gl_renderstate.h"
#include "gl/renderer/gl_renderer.h"
#include "gl/data/gl_vertexbuffer.h"
#include <cstring>
#include <string>
#include <sstream>
#include <memory>
#include <algorithm>

extern "C" {
#include "OVR_CAPI_GL.h"
}

#include "Extras/OVR_Math.h"

using namespace std;

static const char* hudQuadShaderVrtxSrc = ""
"#version 140\n"
"in vec4 position;\n"
"in vec2 texCoord;\n"
"uniform mat4 projectionMatrix = mat4(1);\n"
"uniform mat4 viewMatrix = mat4(1);\n"
"out vec2 fragTexCoord;\n"
"void main() {\n"
"	gl_Position = projectionMatrix * viewMatrix * position;\n"
"	fragTexCoord = texCoord;\n"
"}\n";

static const char* hudQuadShaderFragSrc = ""
"#version 140\n"
"in vec2 fragTexCoord;\n"
"uniform sampler2D hudTexture;\n"
"out vec4 color;\n"
"void main() {\n"
"	vec4 s = texture(hudTexture, fragTexCoord);\n"
"	color = vec4(s.r, s.g, s.b, s.a);\n"
"}\n";

RiftHmd::RiftHmd()
	: hmd(nullptr)
	, sceneFrameBuffer(0)
	, depthBuffer(0)
	, sceneTextureSet(nullptr)
	// , mirrorTexture(nullptr)
	, frameIndex(0)
	, poseOrigin(OVR::Vector3f(0,0,0))
{
}

void RiftHmd::createShaders() {
	int hudQuadShaderVrtx = glCreateShader(GL_VERTEX_SHADER);
	int hudQuadShaderFrag = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(hudQuadShaderVrtx, 1, &hudQuadShaderVrtxSrc, nullptr);
	glShaderSource(hudQuadShaderFrag, 1, &hudQuadShaderFragSrc, nullptr);
	glCompileShader(hudQuadShaderVrtx);
	glCompileShader(hudQuadShaderFrag);
	hudQuadShader = glCreateProgram();
	glAttachShader(hudQuadShader, hudQuadShaderVrtx);
	glAttachShader(hudQuadShader, hudQuadShaderFrag);
	glLinkProgram(hudQuadShader);
	GLsizei infoLength;
	GLchar infoBuffer[1001];
	glGetProgramInfoLog(hudQuadShader, 1000, &infoLength, infoBuffer);
	if (*infoBuffer) {
		fprintf(stderr, "%s", infoBuffer);
	}
	hqsViewMatrixLoc = glGetUniformLocation(hudQuadShader, "viewMatrix");
	hqsProjMatrixLoc = glGetUniformLocation(hudQuadShader, "projectionMatrix");
	hqsHudTextureLoc = glGetUniformLocation(hudQuadShader, "hudTexture");

	// Create my own vertex buffer objects
	GLuint hud_n_xhair_buffers[] = {0, 0};
	glGenBuffers(2, hud_n_xhair_buffers);
	hudVbo = hud_n_xhair_buffers[0];
	GLuint xhairVbo = hud_n_xhair_buffers[1];

	float hudScale = 0.4;
	float hudDistance = 1.56; // meters
	float hudWidth = hudScale * 1.0 / 0.4 * hudDistance;
	float hudHeight = hudWidth * 3.0f / 4.0f;
	float hud_vbo_data[] = {
		-0.5*hudWidth, 0.5*hudHeight, -hudDistance,   0, 1,
		-0.5*hudWidth,-0.5*hudHeight, -hudDistance,   0, 0,
		0.5*hudWidth, 0.5*hudHeight, -hudDistance,   1, 1,
		0.5*hudWidth,-0.5*hudHeight, -hudDistance,   1, 0
	};

	/*
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	int positionLoc = glGetAttribLocation(hudQuadShader, "position");
	int texCoordLoc = glGetAttribLocation(hudQuadShader, "texCoord");
	glEnableVertexAttribArray(positionLoc);
	glEnableVertexAttribArray(texCoordLoc);
	glBindBuffer(GL_ARRAY_BUFFER, hudVbo);
	// glVertexAttribPointer(positionLoc, 3, GL_FLOAT, false, sizeof(FLOAT) * 5, 0);
	// glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT, false, sizeof(FLOAT) * 5, (void*)12);
	glBufferData(GL_ARRAY_BUFFER, 20 * sizeof(float), &hud_vbo_data[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	*/
	
}

void RiftHmd::destroy() {
	if (sceneTextureSet) {
		// ovr_DestroyTextureSwapChain(hmd, sceneTextureSet); // causes hang/crash
		sceneTextureSet = nullptr;
	}
	// Carl: don't shut down unless we initialized
	if (hmd) {
		glDeleteRenderbuffers(1, &depthBuffer);
		depthBuffer = 0;
		glDeleteFramebuffers(1, &sceneFrameBuffer);
		sceneFrameBuffer = 0;
		// ovr_Destroy(hmd); // causes hang/crash
		hmd = nullptr;
	}
	ovr_Shutdown();
}

ovrResult RiftHmd::init_tracking() 
{
	if (hmd) return ovrSuccess; // already initialized
	ovrResult result = ovr_Initialize(nullptr);
	if OVR_FAILURE(result)
		return result;
	ovrGraphicsLuid luid;
	ovr_Create(&hmd, &luid);
	return result;
}


ovrResult RiftHmd::init_graphics() 
{
    // NOTE: Initialize OpenGL first (elsewhere), before getting Rift textures here.

	// HMD
	ovrResult result = init_tracking();
	if OVR_FAILURE(result)
		return result;

	// 3D scene
	result = init_scene_texture();
	if OVR_FAILURE(result)
		return result;

	createShaders();

	return result;
}

ovrResult RiftHmd::init_scene_texture()
{
	if (sceneTextureSet)
		return ovrSuccess;

    // Configure Stereo settings.
    // Use a single shared texture for simplicity
    // 1bb) Compute texture sizes
	ovrHmdDesc hmdDesc = ovr_GetHmdDesc(hmd);
    ovrSizei recommendedTex0Size = ovr_GetFovTextureSize(hmd, ovrEye_Left, 
            hmdDesc.DefaultEyeFov[0], 1.0);
    ovrSizei recommendedTex1Size = ovr_GetFovTextureSize(hmd, ovrEye_Right,
            hmdDesc.DefaultEyeFov[1], 1.0);
    ovrSizei bufferSize;
    bufferSize.w  = recommendedTex0Size.w + recommendedTex1Size.w;
    bufferSize.h = std::max( recommendedTex0Size.h, recommendedTex1Size.h );
    // print "Recommended buffer size = ", bufferSize, bufferSize.w, bufferSize.h
    // NOTE: We need to have set up OpenGL context before this point...
    // 1c) Allocate SwapTextureSets
	ovrTextureSwapChainDesc tscd;
	tscd.Type = ovrTexture_2D;
	tscd.ArraySize = 1;
	tscd.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
	tscd.Width = bufferSize.w;
	tscd.Height = bufferSize.h;
	tscd.MipLevels = 1;
	tscd.SampleCount = 1;
	tscd.StaticImage = ovrFalse;
	tscd.MiscFlags = 0;
	tscd.BindFlags = 0;
    ovrResult result = ovr_CreateTextureSwapChainGL(hmd,
			&tscd,
			&sceneTextureSet);
	if OVR_FAILURE(result)
		return result;

    // Initialize VR structures, filling out description.
    // 1ba) Compute FOV
    ovrEyeRenderDesc eyeRenderDesc[2];
    // ovrVector3f hmdToEyeOffset[2];
    eyeRenderDesc[0] = ovr_GetRenderDesc(hmd, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
    eyeRenderDesc[1] = ovr_GetRenderDesc(hmd, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);
    hmdToEyeOffset[0] = eyeRenderDesc[0].HmdToEyeOffset;
    hmdToEyeOffset[1] = eyeRenderDesc[1].HmdToEyeOffset;

	// Stereo3D Layer for primary 3D scene
    // Initialize our single full screen Fov layer.
    // ovrLayerEyeFov layer;
	sceneLayer.Header.Type      = ovrLayerType_EyeFov;
    sceneLayer.Header.Flags     = 
			// ovrLayerFlag_HighQuality |
			ovrLayerFlag_TextureOriginAtBottomLeft; // OpenGL convention;
    sceneLayer.ColorTexture[0]  = sceneTextureSet; // single texture for both eyes;
    sceneLayer.ColorTexture[1]  = sceneTextureSet; // single texture for both eyes;
    sceneLayer.Fov[0]           = eyeRenderDesc[0].Fov;
    sceneLayer.Fov[1]           = eyeRenderDesc[1].Fov;
	sceneLayer.Viewport[0].Pos.x = 0;
	sceneLayer.Viewport[0].Pos.y = 0;
	sceneLayer.Viewport[0].Size.w = bufferSize.w / 2;
	sceneLayer.Viewport[0].Size.h = bufferSize.h;
	sceneLayer.Viewport[1].Pos.x = bufferSize.w / 2;
	sceneLayer.Viewport[1].Pos.y = 0;
	sceneLayer.Viewport[1].Size.w = bufferSize.w / 2;
	sceneLayer.Viewport[1].Size.h = bufferSize.h;

	// create OpenGL framebuffer for rendering to Rift
	glGenFramebuffers(1, &sceneFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, sceneFrameBuffer);
	// color layer will be provided by Rift API at render time...
	// depth buffer
	glGenRenderbuffers(1, &depthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, bufferSize.w, bufferSize.h);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

	// clean up
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	return result;
}

ovrSizei RiftHmd::getViewSize() {
	return sceneLayer.Viewport[0].Size;
}

void RiftHmd::bindToSceneFrameBuffer() {
	glBindFramebuffer(GL_FRAMEBUFFER, sceneFrameBuffer);
}

bool RiftHmd::bindToSceneFrameBufferAndUpdate()
{
	// glBindFramebuffer(GL_FRAMEBUFFER, 0); HudTexture::hudTexture->renderToScreen(); // OK

	if (sceneFrameBuffer == 0) init_graphics();

	// glBindFramebuffer(GL_FRAMEBUFFER, 0); HudTexture::hudTexture->renderToScreen(); // OK

	glBindFramebuffer(GL_FRAMEBUFFER, sceneFrameBuffer);

	// glBindFramebuffer(GL_FRAMEBUFFER, 0); HudTexture::hudTexture->renderToScreen(); glBindFramebuffer(GL_FRAMEBUFFER, sceneFrameBuffer);
 // OK

    double displayMidpointSeconds = ovr_GetPredictedDisplayTime(hmd, 0);
	double sampleTime = ovr_GetTimeInSeconds(); // for tracking latency

	// HudTexture::hudTexture->renderToScreen(); // too late
	
	ovrTrackingState hmdState = ovr_GetTrackingState(hmd, displayMidpointSeconds, true);
    // print hmdState.HeadPose.ThePose
    ovr_CalcEyePoses(hmdState.HeadPose.ThePose, 
            hmdToEyeOffset,
            sceneLayer.RenderPose);
	sceneLayer.SensorSampleTime = sampleTime;

	// Apply our custom position-but-not-yaw recentering offset
	for (int eye = 0; eye < 2; ++eye) {
		OVR::Vector3f pos = sceneLayer.RenderPose[eye].Position;
		pos -= poseOrigin;
		sceneLayer.RenderPose[eye].Position = pos;
	}

	// HudTexture::hudTexture->renderToScreen(); // too late
	
	// Increment to use next texture, just before writing
    // 2d) Advance CurrentIndex within each used texture set to target the next consecutive texture buffer for the following frame.
	unsigned int textureId;
	ovr_GetTextureSwapChainBufferGL(hmd, sceneTextureSet, -1, &textureId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, 
            GL_COLOR_ATTACHMENT0, 
            GL_TEXTURE_2D,
			textureId,
            0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
	GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	GLenum desiredStatus = GL_FRAMEBUFFER_COMPLETE;

	//glBindFramebuffer(GL_FRAMEBUFFER, 0); HudTexture::hudTexture->renderToScreen(); glBindFramebuffer(GL_FRAMEBUFFER, sceneFrameBuffer);
	// OK

	if (fbStatus != GL_FRAMEBUFFER_COMPLETE)
		return false;
	return true;
}

void RiftHmd::paintHudQuad(float hudScale, float pitchAngle, float yawRange) 
{
	// Place hud relative to torso
	ovrPosef pose = getCurrentEyePose();
	// Convert from Rift camera coordinates to game coordinates
	// float gameYaw = renderer_param.mAngles.Yaw;
	OVR::Quatf hmdRot(pose.Orientation);
	float hmdYaw, hmdPitch, hmdRoll;
	hmdRot.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&hmdYaw, &hmdPitch, &hmdRoll);
	OVR::Quatf yawCorrection(OVR::Vector3f(0, 1, 0), -hmdYaw); // 
	// OVR::Vector3f trans0(pose.Position);
	OVR::Vector3f eyeTrans = yawCorrection.Rotate(pose.Position);

	// Keep HUD fixed relative to the torso, and convert angles to degrees
	float hudPitch = -hmdPitch * 180/3.14159;
	float hudRoll = -hmdRoll * 180/3.14159;
	hmdYaw *= -180/3.14159;

	// But allow hud yaw to vary within a range about torso yaw
	static float hudYaw = 0;
	static bool haveHudYaw = false;
	if (! haveHudYaw) {
		haveHudYaw = true;
		hudYaw = hmdYaw;
	}
	// shift deviation from camera yaw to range +- 180 degrees
	float dYaw = hmdYaw - hudYaw;
	while (dYaw > 180) dYaw -= 360;
	while (dYaw < -180) dYaw += 360;
	// float yawRange = 20;
	if (dYaw < -yawRange) dYaw = -yawRange;
	if (dYaw > yawRange) dYaw = yawRange;
	// Slowly center hud yaw toward view direction
	// 1) Proportional term:
	dYaw *= 0.999;
	// 2) Constant term
	float recenterIncrement = 0.003; // degrees
	if (dYaw >= recenterIncrement) dYaw -= recenterIncrement;
	if (dYaw <= -recenterIncrement) dYaw += recenterIncrement;
	hudYaw = hmdYaw - dYaw;

	// gl_MatrixStack.Push(gl_RenderState.mViewMatrix);
	gl_RenderState.mViewMatrix.loadIdentity();

	gl_RenderState.mViewMatrix.rotate(hudRoll, 0, 0, 1);
	gl_RenderState.mViewMatrix.rotate(hudPitch, 1, 0, 0);
	gl_RenderState.mViewMatrix.rotate(dYaw, 0, 1, 0);

	gl_RenderState.mViewMatrix.rotate(pitchAngle, 1, 0, 0); // place hud below horizon

	gl_RenderState.mViewMatrix.translate(-eyeTrans.x, -eyeTrans.y, -eyeTrans.z);

	float hudDistance = 1.56; // meters
	float hudWidth = hudScale * 1.0 / 0.4 * hudDistance;
	float hudHeight = hudWidth * 3.0f / 4.0f;

	gl_RenderState.ResetColor();
	gl_RenderState.ApplyMatrices();
	gl_RenderState.Apply();

	FFlatVertex *ptr = GLRenderer->mVBO->GetBuffer();
	ptr->Set(-0.5*hudWidth, 0.5*hudHeight, -hudDistance,   0, 1); ptr++;
	ptr->Set(-0.5*hudWidth,-0.5*hudHeight, -hudDistance,   0, 0); ptr++;
	ptr->Set( 0.5*hudWidth, 0.5*hudHeight, -hudDistance,   1, 1); ptr++;
	ptr->Set( 0.5*hudWidth,-0.5*hudHeight, -hudDistance,   1, 0); ptr++;

	// It turns out I did not need to use my own shader
	const bool useMyShader = true;
	if (useMyShader) {
		glUseProgram(hudQuadShader);
		gl_RenderState.mViewMatrix.matrixToGL(hqsViewMatrixLoc);
		gl_RenderState.mProjectionMatrix.matrixToGL(hqsProjMatrixLoc);
		glUniform1i(hqsHudTextureLoc, 0);
	}


	const bool useMyVbos = false;
	if (useMyVbos) {
		glBindBuffer(GL_ARRAY_BUFFER, hudVbo);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 20);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	else {
		GLRenderer->mVBO->RenderCurrent(ptr, GL_TRIANGLE_STRIP);
	}

	// gl_MatrixStack.Pop(gl_RenderState.mViewMatrix);
}

void RiftHmd::paintCrosshairQuad(const ovrPosef& eyePose, const ovrPosef& otherEyePose, bool reducedHud) 
{
	// Place weapon relative to head
	const ovrPosef& pose = eyePose;

	// Position of center between two eyes
	OVR::Vector3f eyeCenter = (OVR::Vector3f(eyePose.Position) + OVR::Vector3f(otherEyePose.Position)) * 0.5;

	// Just the interpupillary shift, without other components of position tracking
	OVR::Vector3f eyeShift = OVR::Vector3f(eyePose.Position) - eyeCenter;

	// Place crosshair relative to head
	// ovrPosef pose = getCurrentEyePose();
	// Convert from Rift camera coordinates to game coordinates
	OVR::Quatf hmdRot(pose.Orientation);
	float hmdYaw, hmdPitch, hmdRoll;
	hmdRot.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&hmdYaw, &hmdPitch, &hmdRoll);
	OVR::Vector3f eyeTrans = hmdRot.InverseRotate(eyeShift);

	// Keep crosshair fixed relative to the head, modulo roll, and convert angles to degrees
	float hudRoll = -hmdRoll * 180/3.14159;

	gl_RenderState.mModelMatrix.loadIdentity();
	gl_RenderState.mViewMatrix.loadIdentity();
	gl_RenderState.mViewMatrix.translate(-eyeTrans.x, 0, 0);
	// Correct Roll, but not pitch nor yaw
	gl_RenderState.mViewMatrix.rotate(hudRoll, 0, 0, 1);

	// TODO: set crosshair distance by reading 3D depth map texture
	float hudDistance = 25.0; // meters? looks closer than that...
	float extra_padding_factor = 2.0; // Room around edges for larger crosshairs
	float hudWidth = extra_padding_factor * 0.075 * hudDistance; // About right size for largest crosshair
	float hudHeight = hudWidth;
	// Bigger number makes a smaller crosshair
	const float txw = extra_padding_factor * 0.040; // half width of quad in texture coordinates; big enough to hold largest crosshair
	const float txh = txw * 4.0/3.0;
	float yCenter = 0.5;
	if (reducedHud)
		yCenter = 0.58;

	gl_RenderState.SetColor(1, 1, 1, 0.5);
	gl_RenderState.ApplyMatrices();
	gl_RenderState.Apply();
	FFlatVertex *ptr = GLRenderer->mVBO->GetBuffer();
	ptr->Set(-0.5*hudWidth,  0.5*hudHeight, -hudDistance,  0.5 - txw, yCenter + txh); ptr++;
	ptr->Set(-0.5*hudWidth, -0.5*hudHeight, -hudDistance,  0.5 - txw, yCenter - txh); ptr++;
	ptr->Set( 0.5*hudWidth,  0.5*hudHeight, -hudDistance,  0.5 + txw, yCenter + txh); ptr++;
	ptr->Set( 0.5*hudWidth, -0.5*hudHeight, -hudDistance,  0.5 + txw, yCenter - txh); ptr++;
	GLRenderer->mVBO->RenderCurrent(ptr, GL_TRIANGLE_STRIP);
}

void RiftHmd::paintWeaponQuad(const ovrPosef& eyePose, const ovrPosef& otherEyePose, float weaponDist, float weaponHeight) 
{
	// Place weapon relative to head
	const ovrPosef& pose = eyePose;

	// Position of center between two eyes
	OVR::Vector3f eyeCenter = (OVR::Vector3f(eyePose.Position) + OVR::Vector3f(otherEyePose.Position)) * 0.5;

	// Just the interpupillary shift, without other components of position tracking
	OVR::Vector3f eyeShift = OVR::Vector3f(eyePose.Position) - eyeCenter;

	// Cause gun to lag behind head position
	// otherShift = emainder of positional offset, aside from interpupillary offset
	OVR::Vector3f otherShift = OVR::Vector3f(eyePose.Position) - eyeShift;
	// With a time delay, recenter weapon in current positional view
	static OVR::Vector3f movingOrigin = OVR::Vector3f(0, 0, 0);
	OVR::Vector3f dw = otherShift - movingOrigin;
	movingOrigin += dw * 0.03; // Set rate of update here

	// Convert from Rift camera coordinates to game coordinates
	OVR::Quatf hmdRot(pose.Orientation);
	float hmdYaw, hmdPitch, hmdRoll;
	hmdRot.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&hmdYaw, &hmdPitch, &hmdRoll);
	OVR::Vector3f eyeTrans = hmdRot.InverseRotate(eyeShift /* + dw */ ); // Camera relative X/Y/Z

	// Keep crosshair fixed relative to the head, modulo roll, and convert angles to degrees
	float hudRoll = -hmdRoll * 180/3.14159;

	gl_RenderState.mModelMatrix.loadIdentity();
	gl_RenderState.mViewMatrix.loadIdentity();

	gl_RenderState.mViewMatrix.translate(-eyeTrans.x, -eyeTrans.y, -eyeTrans.z); // Stereo only...

	// Correct Roll, but not pitch nor yaw
	gl_RenderState.mViewMatrix.rotate(hudRoll, 0, 0, 1);
	gl_RenderState.mViewMatrix.rotate(weaponHeight, 1, 0, 0);

	float hudDistance = weaponDist; // meters, (measured 46 cm to stock of hand weapon)
	float hudWidth = 0.6; // meters, Adjust for good average weapon size
	float hudHeight = 3.0 / 4.0 * hudWidth;

	gl_RenderState.ResetColor();
	gl_RenderState.ApplyMatrices();
	gl_RenderState.Apply();
	FFlatVertex *ptr = GLRenderer->mVBO->GetBuffer();
	ptr->Set(-0.5*hudWidth, 0.5*hudHeight, -hudDistance, 0, 1); ptr++;
	ptr->Set(-0.5*hudWidth, -0.5*hudHeight, -hudDistance, 0, 0); ptr++;
	ptr->Set(0.5*hudWidth, 0.5*hudHeight, -hudDistance, 1, 1); ptr++;
	ptr->Set(0.5*hudWidth, -0.5*hudHeight, -hudDistance, 1, 0); ptr++;
	GLRenderer->mVBO->RenderCurrent(ptr, GL_TRIANGLE_STRIP);
}

void RiftHmd::paintBlendQuad()
{
	// gl_MatrixStack.Push(gl_RenderState.mModelMatrix);
	// gl_MatrixStack.Push(gl_RenderState.mViewMatrix);
	// gl_MatrixStack.Push(gl_RenderState.mProjectionMatrix);

	gl_RenderState.mModelMatrix.loadIdentity();
	gl_RenderState.mViewMatrix.loadIdentity();

	const float rectSize = 1.0; // how much of the screen should this blend effect take up?

	gl_RenderState.ResetColor();
	gl_RenderState.ApplyMatrices();
	gl_RenderState.Apply();
	FFlatVertex *ptr = GLRenderer->mVBO->GetBuffer();
	ptr->Set(-rectSize, rectSize, -1, 0, 1); ptr++;
	ptr->Set(-rectSize, -rectSize, -1, 0, 0); ptr++;
	ptr->Set(rectSize, rectSize, -1, 1, 1); ptr++;
	ptr->Set(rectSize, -rectSize, -1, 1, 0); ptr++;
	GLRenderer->mVBO->RenderCurrent(ptr, GL_TRIANGLE_STRIP);

	// gl_MatrixStack.Pop(gl_RenderState.mProjectionMatrix);
	// gl_MatrixStack.Pop(gl_RenderState.mViewMatrix);
	// gl_MatrixStack.Pop(gl_RenderState.mModelMatrix);
}

ovrPosef& RiftHmd::setSceneEyeView(int eye, float zNear, float zFar) {
    // Set up eye viewport
    ovrRecti v = sceneLayer.Viewport[eye];
    glViewport(v.Pos.x, v.Pos.y, v.Size.w, v.Size.h);
	glEnable(GL_SCISSOR_TEST);
    glScissor(v.Pos.x, v.Pos.y, v.Size.w, v.Size.h);
    // Get projection matrix for the Rift camera
	gl_RenderState.mProjectionMatrix.loadIdentity();
    ovrMatrix4f proj = ovrMatrix4f_Projection(sceneLayer.Fov[eye], zNear, zFar,
                    ovrProjection_ClipRangeOpenGL);
	ovrMatrix4f proj_Transpose;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			proj_Transpose.M[i][j] = proj.M[j][i];
		}
	}
	gl_RenderState.mProjectionMatrix.multMatrix(&proj_Transpose.M[0][0]);

    // Get view matrix for the Rift camera
	gl_RenderState.mViewMatrix.loadIdentity();
    currentEyePose = sceneLayer.RenderPose[eye];

	return currentEyePose;
}

ovrResult RiftHmd::commitFrame() {
	ovrResult result = ovr_CommitTextureSwapChain(hmd, sceneTextureSet);
	return result;
}

ovrResult RiftHmd::submitFrame(float metersPerSceneUnit) {
    // 2c) Call ovr_SubmitFrame, passing swap texture set(s) from the previous step within a ovrLayerEyeFov structure. Although a single layer is required to submit a frame, you can use multiple layers and layer types for advanced rendering. ovr_SubmitFrame passes layer textures to the compositor which handles distortion, timewarp, and GPU synchronization before presenting it to the headset. 
    ovrViewScaleDesc viewScale;
    viewScale.HmdSpaceToWorldScaleInMeters = metersPerSceneUnit;
    viewScale.HmdToEyeOffset[0] = hmdToEyeOffset[0];
    viewScale.HmdToEyeOffset[1] = hmdToEyeOffset[1];
	ovrLayerHeader* layerList[1];
	layerList[0] = &sceneLayer.Header;
    ovrResult result = ovr_SubmitFrame(hmd, frameIndex, &viewScale, layerList, 1);
    frameIndex += 1;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return result;
}

void RiftHmd::recenter_pose() {
    ovrTrackingState hmdState = ovr_GetTrackingState(hmd, frameIndex, false);
	poseOrigin = hmdState.HeadPose.ThePose.Position;
	// ovr_RecenterPose(hmd);
}

static RiftHmd _sharedRiftHmd;
RiftHmd* sharedRiftHmd = &_sharedRiftHmd;

