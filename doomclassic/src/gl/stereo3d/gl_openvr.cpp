/*
** gl_openvr.cpp
** Stereoscopic virtual reality mode for the HTC Vive headset
**
**---------------------------------------------------------------------------
** Copyright 2016 Christopher Bruns
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include "gl/stereo3d/gl_openvr.h"
#include "openvr.h"
#include <string>
#include "doomtype.h" // Printf
#include "d_player.h"
#include "g_game.h" // G_Add...
#include "p_local.h" // P_TryMove
#include "r_utility.h" // viewpitch
#include "gl/renderer/gl_renderer.h"
#include "gl/system/gl_system.h"
#include "gl/data/gl_data.h"
#include "c_cvars.h"
#include "LSMatrix.h"

EXTERN_CVAR(Int, screenblocks);

using namespace vr;

namespace s3d 
{

static HmdVector3d_t eulerAnglesFromQuat(HmdQuaternion_t quat) {
	double q0 = quat.w;
	// permute axes to make "Y" up/yaw
	double q2 = quat.x;
	double q3 = quat.y;
	double q1 = quat.z;

	// http://stackoverflow.com/questions/18433801/converting-a-3x3-matrix-to-euler-tait-bryan-angles-pitch-yaw-roll
	double roll = atan2(2 * (q0*q1 + q2*q3), 1 - 2 * (q1*q1 + q2*q2));
	double pitch = asin(2 * (q0*q2 - q3*q1));
	double yaw = atan2(2 * (q0*q3 + q1*q2), 1 - 2 * (q2*q2 + q3*q3));

	return HmdVector3d_t{ yaw, pitch, roll };
}

static HmdQuaternion_t quatFromMatrix(HmdMatrix34_t matrix) {
	HmdQuaternion_t q;
	typedef float f34[3][4];
	f34& a = matrix.m;
	// http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/
	float trace = a[0][0] + a[1][1] + a[2][2]; // I removed + 1.0f; see discussion with Ethan
	if (trace > 0) {// I changed M_EPSILON to 0
		float s = 0.5f / sqrtf(trace + 1.0f);
		q.w = 0.25f / s;
		q.x = (a[2][1] - a[1][2]) * s;
		q.y = (a[0][2] - a[2][0]) * s;
		q.z = (a[1][0] - a[0][1]) * s;
	}
	else {
		if (a[0][0] > a[1][1] && a[0][0] > a[2][2]) {
			float s = 2.0f * sqrtf(1.0f + a[0][0] - a[1][1] - a[2][2]);
			q.w = (a[2][1] - a[1][2]) / s;
			q.x = 0.25f * s;
			q.y = (a[0][1] + a[1][0]) / s;
			q.z = (a[0][2] + a[2][0]) / s;
		}
		else if (a[1][1] > a[2][2]) {
			float s = 2.0f * sqrtf(1.0f + a[1][1] - a[0][0] - a[2][2]);
			q.w = (a[0][2] - a[2][0]) / s;
			q.x = (a[0][1] + a[1][0]) / s;
			q.y = 0.25f * s;
			q.z = (a[1][2] + a[2][1]) / s;
		}
		else {
			float s = 2.0f * sqrtf(1.0f + a[2][2] - a[0][0] - a[1][1]);
			q.w = (a[1][0] - a[0][1]) / s;
			q.x = (a[0][2] + a[2][0]) / s;
			q.y = (a[1][2] + a[2][1]) / s;
			q.z = 0.25f * s;
		}
	}

	return q;
}

static HmdVector3d_t eulerAnglesFromMatrix(HmdMatrix34_t mat) {
	return eulerAnglesFromQuat(quatFromMatrix(mat));
}


/* static */
const OpenVRMode& OpenVRMode::getInstance()
{
	static OpenVRMode instance;
	return instance;
}


OpenVREyePose::OpenVREyePose(vr::EVREye eye)
	: ShiftedEyePose( 0.0f )
	, eye(eye)
	, eyeTexture(nullptr)
	, verticalDoomUnitsPerMeter(27.0f)
	, currentPose(nullptr)
{
}


/* virtual */
OpenVREyePose::~OpenVREyePose() 
{
	dispose();
}


/* virtual */
GL_IRECT* OpenVREyePose::GetViewportBounds(GL_IRECT* bounds) const
{
	static GL_IRECT viewportBounds;
	viewportBounds.left = 0;
	viewportBounds.top = 0;
	viewportBounds.width = framebuffer.getWidth();
	viewportBounds.height = framebuffer.getHeight();
	return &viewportBounds;
}


static void vSMatrixFromHmdMatrix34(VSMatrix& m1, const vr::HmdMatrix34_t& m2)
{
	float tmp[16];
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 4; ++j) {
			tmp[4 * i + j] = m2.m[i][j];
		}
	}
	int i = 3;
	for (int j = 0; j < 4; ++j) {
		tmp[4 * i + j] = 0;
	}
	tmp[15] = 1;
	m1.loadMatrix(&tmp[0]);
}


/* virtual */
void OpenVREyePose::GetViewShift(FLOATTYPE yaw, FLOATTYPE outViewShift[3], sector_t* viewsector) const
{
	if (currentPose == nullptr)
		return;
	const vr::TrackedDevicePose_t& hmd = *currentPose;
	if (! hmd.bDeviceIsConnected)
		return;
	if (! hmd.bPoseIsValid)
		return;
	const vr::HmdMatrix34_t& hmdPose = hmd.mDeviceToAbsoluteTracking;

	// Pitch and Roll are identical between OpenVR and Doom worlds.
	// But yaw can differ, depending on starting state, and controller movement.
	float doomYawDegrees = GLRenderer->mAngles.Yaw.Degrees;
	float openVrYawDegrees = -eulerAnglesFromMatrix(hmdPose).v[0] * 180.0 / 3.14159;
	float deltaYawDegrees = doomYawDegrees - openVrYawDegrees;
	while (deltaYawDegrees > 180)
		deltaYawDegrees -= 360;
	while (deltaYawDegrees < -180)
		deltaYawDegrees += 360;

	// Printf("delta yaw = %.0f; yaw = %.0f\n", deltaYawDegrees, openVrYawDegrees);

	// extract rotation component from hmd transform
	LSMatrix44 hmdLs(hmdPose);
	LSMatrix44 hmdRot = hmdLs.getWithoutTranslation(); // .transpose();

	/// In these eye methods, just get local inter-eye stereoscopic shift, not full position shift ///

	// compute local eye shift
	LSMatrix44 eyeShift2;
	eyeShift2.loadIdentity();
	eyeShift2 = eyeShift2 * eyeToHeadTransform; // eye to head
	eyeShift2 = eyeShift2 * hmdRot; // head to openvr
	eyeShift2.rotate(deltaYawDegrees, 0, 1, 0); // openvr to doom

	// OpenVR knows exactly where the floor is, so...
	// ...set viewz to exact absolute height above the floor
	float openvrHeightMeters = hmdLs[1][3] + eyeShift2[1][3];
	float doomFloorZ = FIXED2FLOAT(viewsector->floorplane.ZatPoint(viewx, viewy));
	float doomViewZDoomUnits = verticalDoomUnitsPerMeter * openvrHeightMeters + doomFloorZ;
	float deltaViewZ = doomViewZDoomUnits - FIXED2FLOAT(viewz);

	float horizontalDoomUnitsPerMeter = verticalDoomUnitsPerMeter * glset.pixelstretch;

	// output per-eye shifts, for stereoscopic rendering by caller of this method
	outViewShift[0] = eyeShift2[0][3] * horizontalDoomUnitsPerMeter;
	outViewShift[1] = eyeShift2[2][3] * horizontalDoomUnitsPerMeter; // "viewy" is horizontal direction in doom
	outViewShift[2] = deltaViewZ; // Set absolute height above; "viewz" is height in doom

	// Printf("viewshift = %2.1f, %2.1f\n", outViewShift[0], outViewShift[1]);
}

/* virtual */
VSMatrix OpenVREyePose::GetProjection(FLOATTYPE fov, FLOATTYPE aspectRatio, FLOATTYPE fovRatio) const
{
	// Ignore those arguments and get the projection from the SDK
	VSMatrix vs1 = ShiftedEyePose::GetProjection(fov, aspectRatio, fovRatio);
	return projectionMatrix;
}


/* virtual */
void OpenVREyePose::SetUp() const
{
	super::SetUp();

	// cachedViewZ = viewz;

	// bind framebuffer
	framebuffer.bindRenderBuffer();

	// just for testing
	// glClearColor(1, 0.5f, 0.5f, 0);
	// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

/* virtual */
void OpenVREyePose::TearDown() const
{
	// viewz = cachedViewZ;

	super::TearDown();
}

void OpenVREyePose::initialize(vr::IVRSystem& vrsystem)
{
	float zNear = 5.0;
	float zFar = 65536.0;
	vr::HmdMatrix44_t projection = vrsystem.GetProjectionMatrix(
			eye, zNear, zFar, vr::API_OpenGL);
	vr::HmdMatrix44_t proj_transpose;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			proj_transpose.m[i][j] = projection.m[j][i];
		}
	}
	projectionMatrix.loadIdentity();
	projectionMatrix.multMatrix(&proj_transpose.m[0][0]);


	vr::HmdMatrix34_t eyeToHead = vrsystem.GetEyeToHeadTransform(eye);
	vSMatrixFromHmdMatrix34(eyeToHeadTransform, eyeToHead);

	uint32_t w, h;
	vrsystem.GetRecommendedRenderTargetSize(&w, &h);

	// create frame buffers
	framebuffer.initialize(w, h);

	if (eyeTexture == nullptr)
		eyeTexture = new vr::Texture_t();
	eyeTexture->handle = (void*)framebuffer.getRenderTextureId(); // TODO: use resolveTextureId, after I implement warping
	eyeTexture->eType = vr::API_OpenGL;
	eyeTexture->eColorSpace = vr::ColorSpace_Gamma;
}


void OpenVREyePose::dispose()
{
	framebuffer.dispose();
	delete eyeTexture;
	eyeTexture = nullptr;
}


bool OpenVREyePose::submitFrame() const
{
	if (eyeTexture == nullptr)
		return false;
	vr::VRCompositor()->Submit(eye, eyeTexture);
	return true;
}


OpenVRMode::OpenVRMode() 
	: ivrSystem(nullptr)
	, leftEyeView(vr::Eye_Left)
	, rightEyeView(vr::Eye_Right)
{
	eye_ptrs.Push(&leftEyeView); // default behavior to Mono non-stereo rendering

	EVRInitError eError;
	if (VR_IsHmdPresent())
	{
		ivrSystem = VR_Init(&eError, VRApplication_Scene);
		if (eError != vr::VRInitError_None) {
			std::string errMsg = VR_GetVRInitErrorAsEnglishDescription(eError);
			ivrSystem = nullptr;
			return;
			// TODO: report error
		}
		// OK
		leftEyeView.initialize(*ivrSystem);
		rightEyeView.initialize(*ivrSystem);

		if (!vr::VRCompositor())
			return;

		eye_ptrs.Push(&rightEyeView); // NOW we render to two eyes
	}
}


// TODO: obsolete method
void OpenVRMode::updateDoomViewDirection() const
{
	if (ivrSystem == nullptr)
		return;
	// Compute how far in the future to predict HMD pose
	float secondsSinceLastVsync = 0;
	ivrSystem->GetTimeSinceLastVsync(&secondsSinceLastVsync, nullptr);
	float displayFrequency = ivrSystem->GetFloatTrackedDeviceProperty(k_unTrackedDeviceIndex_Hmd, Prop_DisplayFrequency_Float);
	float frameDuration = 1.0f / displayFrequency;
	float vsyncToPhotons = ivrSystem->GetFloatTrackedDeviceProperty(k_unTrackedDeviceIndex_Hmd, Prop_SecondsFromVsyncToPhotons_Float);
	float predictedSecondsFromNow = frameDuration - secondsSinceLastVsync + vsyncToPhotons;

	// Compute HMD pose, in terms of yaw, pitch, and roll
	TrackedDevicePose_t trackedDevicePoses[k_unMaxTrackedDeviceCount];
	ivrSystem->GetDeviceToAbsoluteTrackingPose(
		TrackingUniverseStanding,
		predictedSecondsFromNow,
		trackedDevicePoses,
		k_unMaxTrackedDeviceCount
	);
	TrackedDevicePose_t& hmdPose = trackedDevicePoses[k_unTrackedDeviceIndex_Hmd];

	HmdVector3d_t eulerAngles = eulerAnglesFromMatrix(hmdPose.mDeviceToAbsoluteTracking);
	// Printf("%.1f %.1f %.1f\n", eulerAngles.v[0], eulerAngles.v[1], eulerAngles.v[2]);

	if (hmdPose.bPoseIsValid)
		updateHmdPose(eulerAngles.v[0], eulerAngles.v[1], eulerAngles.v[2]);
}


void OpenVRMode::updateHmdPose(
	double hmdYawRadians, 
	double hmdPitchRadians, 
	double hmdRollRadians) const 
{
	double hmdyaw = hmdYawRadians;
	double hmdpitch = hmdPitchRadians;
	double hmdroll = hmdRollRadians;

	// Set HMD angle game state parameters for NEXT frame
	static double previousYaw = 0;
	static bool havePreviousYaw = false;
	if (!havePreviousYaw) {
		previousYaw = hmdyaw;
		havePreviousYaw = true;
	}
	double dYaw = hmdyaw - previousYaw;
	G_AddViewAngle((int)(-32768.0*dYaw / 3.14159)); // determined empirically
	previousYaw = hmdyaw;

	// Pitch
	float doomPitch = atan(tan(hmdpitch) / glset.pixelstretch);
	int pitch = (int)(-32768 / 3.14159*doomPitch);
	int dPitch = (pitch - viewpitch / 65536); // empirical
	G_AddViewPitch(-dPitch);

	// Roll can be local, because it doesn't affect gameplay.
	GLRenderer->mAngles.Roll = -hmdroll * 180.0 / 3.14159;

	// Late-schedule update to renderer angles directly, too
	bool doLateScheduledRotationTracking = true;
	if (doLateScheduledRotationTracking) {
		GLRenderer->mAngles.Pitch = -hmdpitch * 180.0 / 3.14159;
		GLRenderer->mAngles.Yaw += dYaw * 180.0 / 3.14159; // "plus" is the correct direction
	}
}

/* virtual */
void OpenVRMode::SetUp(player_t* player) const
{
	super::SetUp(player);

	cachedViewX = viewx;
	cachedViewY = viewy;

	cachedScreenBlocks = screenblocks;
	screenblocks = 12; // always be full-screen during 3D scene render

	if (vr::VRCompositor() == nullptr)
		return;

	static vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount];
	vr::VRCompositor()->WaitGetPoses(
		poses, vr::k_unMaxTrackedDeviceCount, // current pose
		nullptr, 0 // future pose?
	);

	TrackedDevicePose_t& hmdPose0 = poses[vr::k_unTrackedDeviceIndex_Hmd];

	if (hmdPose0.bPoseIsValid) {
		const vr::HmdMatrix34_t& hmdPose = hmdPose0.mDeviceToAbsoluteTracking;
		HmdVector3d_t eulerAngles = eulerAnglesFromMatrix(hmdPose);
		// Printf("%.1f %.1f %.1f\n", eulerAngles.v[0], eulerAngles.v[1], eulerAngles.v[2]);
		updateHmdPose(eulerAngles.v[0], eulerAngles.v[1], eulerAngles.v[2]);
		leftEyeView.setCurrentHmdPose(&hmdPose0);
		rightEyeView.setCurrentHmdPose(&hmdPose0);

		// TODO: position tracking in the horizontal direction
		// We need to track incremental changes in three locations:
		// 1) the measured absolute hmd location
		// 2) the game player view location
		// 3) the actual view location we used in this method
		static bool haveOldLocations = false;
		// All these values are in meters :)
		static float oldHmdX, oldHmdZ; // doom units, rotated to doom frame
		static float oldPlayerX, oldPlayerZ; // doom units, float
		static float  oldViewX, oldViewZ; // doom units, float

		// Construct translation-only version of hmd transform
		// Pitch and Roll are identical between OpenVR and Doom worlds.
		// But yaw can differ, depending on starting state, and controller movement.
		float doomYawDegrees = GLRenderer->mAngles.Yaw.Degrees;
		float openVrYawDegrees = -eulerAnglesFromMatrix(hmdPose).v[0] * 180.0 / 3.14159;
		float deltaYawDegrees = doomYawDegrees - openVrYawDegrees;
		while (deltaYawDegrees > 180)
			deltaYawDegrees -= 360;
		while (deltaYawDegrees < -180)
			deltaYawDegrees += 360;

		// extract rotation component from hmd transform
		LSMatrix44 hmdLs(hmdPose);
		LSMatrix44 hmdRot = hmdLs.getWithoutTranslation(); // .transpose();
		LSMatrix44 hmdPos;
		hmdPos.loadIdentity();
		hmdPos[0][3] = hmdLs[0][3];
		hmdPos[1][3] = hmdLs[1][3];
		hmdPos[2][3] = hmdLs[2][3];
		// Rotate hmd translation to match doom frame
		hmdPos.rotate(deltaYawDegrees, 0, 1, 0); // openvr to doom
		float verticalDoomUnitsPerMeter = 27.0f; // TODO: abstract this
		float horizontalDoomUnitsPerMeter = verticalDoomUnitsPerMeter * glset.pixelstretch;
		float newHmdX = hmdPos[0][3] * horizontalDoomUnitsPerMeter;
		float newHmdZ = hmdPos[2][3] * horizontalDoomUnitsPerMeter;
		float newPlayerX = FIXED2FLOAT(player->mo->X());
		float newPlayerZ = FIXED2FLOAT(player->mo->Y());
		float newViewX = newPlayerX;
		float newViewZ = newPlayerZ;
		if (haveOldLocations) {
			float deltaHmdX = newHmdX - oldHmdX;
			float deltaHmdZ = newHmdZ - oldHmdZ;
			float deltaPlayerX = newPlayerX - oldPlayerX;
			float deltaPlayerZ = newPlayerZ - oldPlayerZ;

			// Let's see, if only the hmd changed, but not the player, we
			// still definitely want to shift the view, because the player
			// moved their head. And we should update the player position for next tic.
			// 
			// If only the player moved, but not the hmd, I guess we ALSO follow
			// the move, because (maybe) the player moved the controller or something.
			// 
			// If BOTH move the same, I guess we just move the hmd amount.
			newViewX = oldViewX - deltaHmdX + deltaPlayerX; // TODO:
			newViewZ = oldViewZ + deltaHmdZ + deltaPlayerZ; // TODO:

			// try to move player to new view location...
			float xDiscrepancy = newViewX - newPlayerX;
			float zDiscrepancy = newViewZ - newPlayerZ;
			float absDiscrepancy = sqrt(xDiscrepancy*xDiscrepancy + zDiscrepancy*zDiscrepancy);

			float teleportThreshold = 1.0 * horizontalDoomUnitsPerMeter;
			float tetherThreshold = 0.5 * horizontalDoomUnitsPerMeter;

			// Feature toggles
			bool doTeleportViewpoint = true;
			bool doTetherViewpoint = true;
			bool doFollowHmdLocation = true;
			bool doLateScheduledPositionTracking = true;

			// Is our viewpoint too far from the true player location?
			if (doTeleportViewpoint && (absDiscrepancy >= teleportThreshold)) {
				// Teleport viewpoint to actual player location
				newViewX = newPlayerX;
				newViewZ = newPlayerZ;
				xDiscrepancy = zDiscrepancy = absDiscrepancy = 0;
			}
			else if (doFollowHmdLocation) {
				// Move in-game player pawn to new position, if possible
				// player->mo->Move(FLOAT2FIXED(xDiscrepancy), FLOAT2FIXED(zDiscrepancy), 0);
				// player->mo->SetOrigin(FLOAT2FIXED(newViewX), FLOAT2FIXED(newViewZ), viewy, true);
				if (P_TryMove(player->mo, FLOAT2FIXED(newViewX), FLOAT2FIXED(newViewZ), true, nullptr))
				{
					// Let's hope player location updated already...
					newPlayerX = FIXED2FLOAT(player->mo->X());
					newPlayerZ = FIXED2FLOAT(player->mo->Y());
					deltaPlayerX = newPlayerX - oldPlayerX;
					deltaPlayerZ = newPlayerZ - oldPlayerZ;
					float xDiscrepancy = newViewX - newPlayerX;
					float zDiscrepancy = newViewZ - newPlayerZ;
					float absDiscrepancy = sqrt(xDiscrepancy*xDiscrepancy + zDiscrepancy*zDiscrepancy);
				}
			}

			// Prevent viewpoint from moving more than a certain amount away from true in-game player location
			if (doTetherViewpoint && (absDiscrepancy >= tetherThreshold)) {
				float tetherScale = tetherThreshold / absDiscrepancy;
				newViewX = newPlayerX + tetherScale * xDiscrepancy;
				newViewZ = newPlayerZ + tetherScale * zDiscrepancy;
			}

			if (doLateScheduledPositionTracking)
			{
				viewx = FLOAT2FIXED(newViewX); // OK
				viewy = FLOAT2FIXED(newViewZ); // OK
			}

			// Printf("dxz = %.1f, %.1f (%.1f)\n", xDiscrepancy, zDiscrepancy, absDiscrepancy);

		}
		else {
			haveOldLocations = true;
		}
		oldHmdX = newHmdX;
		oldHmdZ = newHmdZ;
		oldPlayerX = newPlayerX;
		oldPlayerZ = newPlayerZ;
		oldViewX = newViewX;
		oldViewZ = newViewZ;
	}
}

/* virtual */
void OpenVRMode::TearDown() const
{
	screenblocks = cachedScreenBlocks;
	viewy = cachedViewY;
	viewx = cachedViewX;

	// Unbind eye texture framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	leftEyeView.submitFrame();
	rightEyeView.submitFrame();
	glFinish();

	// TODO: bind Hud framebuffer

	super::TearDown();
}

/* virtual */
OpenVRMode::~OpenVRMode() 
{
	if (ivrSystem != nullptr) {
		VR_Shutdown();
		ivrSystem = nullptr;
		leftEyeView.dispose();
		rightEyeView.dispose();
	}
}

} /* namespace s3d */
