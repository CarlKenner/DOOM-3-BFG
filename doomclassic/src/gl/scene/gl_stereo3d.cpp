#include "v_video.h"
#include "c_dispatch.h"
#include "gl/system/gl_interface.h"
#include "gl/system/gl_system.h"
#include "gl/system/gl_cvars.h"
#include "gl/system/gl_framebuffer.h"
#include "gl/scene/gl_stereo3d.h"
#include "gl/renderer/gl_renderer.h"
#include "gl/renderer/gl_renderstate.h"
#include "gl/renderer/gl_colormap.h"
#include "gl/scene/gl_colormask.h"
#include "gl/scene/gl_hudtexture.h"
#include "gl/utility/gl_clock.h"
#include "gl/utility/gl_convert.h"
#include "doomstat.h"
#include "d_player.h"
#include "r_utility.h" // viewpitch
#include "g_game.h"
#include "c_console.h"
#include "sbar.h"
#include "am_map.h"
#include "gl/scene/gl_localhudrenderer.h"
#include <cmath>

#include "Extras/OVR_Math.h"

extern void P_CalcHeight (player_t *player);

extern DBaseStatusBar *StatusBar; // To access crosshair drawing


EXTERN_CVAR(Bool, vid_vsync)
EXTERN_CVAR(Float, vr_screendist)
//
CVAR(Int, vr_mode, 0, CVAR_GLOBALCONFIG)
CVAR(Bool, vr_swap, false, CVAR_GLOBALCONFIG)
// intraocular distance in meters
CVAR(Float, vr_ipd, 0.062f, CVAR_ARCHIVE|CVAR_GLOBALCONFIG) // METERS
CVAR(Float, vr_rift_fov, 117.4f, CVAR_ARCHIVE|CVAR_GLOBALCONFIG) // DEGREES
CVAR(Float, vr_view_yoffset, 4.0, 0) // MAP UNITS - raises your head to be closer to soldier height
// Especially Oculus Rift VR geometry depends on exact mapping between doom map units and real world.
// Supposed to be 32 units per meter, according to http://doom.wikia.com/wiki/Map_unit
// But ceilings and floors look too close at that scale.
CVAR(Float, vr_player_height_meters, 1.75f, CVAR_ARCHIVE|CVAR_GLOBALCONFIG) // Used for stereo 3D
// CVAR(Float, vr_rift_aspect, 640.0/800.0, CVAR_GLOBALCONFIG) // Used for stereo 3D
CVAR(Float, vr_weapon_height, 0.0, CVAR_ARCHIVE|CVAR_GLOBALCONFIG) // Used for oculus rift
CVAR(Float, vr_weapondist, 0.45, CVAR_ARCHIVE|CVAR_GLOBALCONFIG) // METERS
CVAR(Int, vr_device, 1, CVAR_GLOBALCONFIG) // 1 for DK1, 2 for DK2 (Default to DK2)
CVAR(Float, vr_sprite_scale, 0.40, CVAR_ARCHIVE|CVAR_GLOBALCONFIG) // weapon size
CVAR(Float, vr_hud_scale, 0.4, CVAR_ARCHIVE|CVAR_GLOBALCONFIG) // menu/message size
// CVAR(Bool, vr_lowpersist, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
// For broadest GL compatibility, require user to explicitly enable quad-buffered stereo mode.
// Setting vr_enable_quadbuffered_stereo does not automatically invoke quad-buffered stereo,
// but makes it possible for subsequent "vr_mode 7" to invoke quad-buffered stereo
CVAR(Bool, vr_enable_quadbuffered, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)

// Command to set "standard" rift settings
EXTERN_CVAR(Int, con_scaletext)
EXTERN_CVAR(Bool, hud_scale)
EXTERN_CVAR(Int, hud_althudscale)
EXTERN_CVAR(Bool, crosshairscale)
EXTERN_CVAR(Bool, freelook)
EXTERN_CVAR(Float, movebob)
EXTERN_CVAR(Float, turbo)
EXTERN_CVAR(Int, screenblocks)
EXTERN_CVAR(Int, m_use_mouse)
EXTERN_CVAR(Int, crosshair)
EXTERN_CVAR(Bool, smooth_mouse)
EXTERN_CVAR(Int, wipetype)
CCMD(oculardium_optimosa)
{
	// scale up all HUD chrome
	con_scaletext = 1; // console and messages
	hud_scale = 1;
	hud_althudscale = 1;
	crosshairscale = 1;
	movebob = 0.05; // No bobbing
	turbo = 80; // Slower walking
	vr_mode = 8; // Rift mode
	// Use minimal or no HUD
	// if (screenblocks <= 10)
	// 	screenblocks = 11;
	// vr_lowpersist = true;
	m_use_mouse = 0; // no mouse in menus
	// freelook = false; // no up/down look with mouse // too intrusive?
	crosshair = 1; // show crosshair
	vr_view_yoffset = 4;
	// AddCommandString("vid_setmode 1920 1080 32\n"); // causes crash
	smooth_mouse = 0;
	// prevent mirror-based judder
	vid_vsync = false;
	wipetype = 0; // until I can fix the wipe glitches...
}


// Create aliases for comfort mode controls
// Quick turn commands for VR comfort mode
static void set_turn_aliases() {
	static bool b_turn_aliases_set = false;
	if (b_turn_aliases_set) return;
	AddCommandString("alias turn45left \"alias turn45_step \\\"wait 5;-left;turnspeeds 640 1280 320 320;alias turn45_step\\\";turn45_step;wait;turnspeeds 2048 2048 2048 2048;+left\"\n");
	AddCommandString("alias turn45right \"alias turn45_step \\\"wait 5;-right;turnspeeds 640 1280 320 320;alias turn45_step\\\";turn45_step;wait;turnspeeds 2048 2048 2048 2048;+right\"\n");
	b_turn_aliases_set = true;
}
// These commands snap45left snap45right are linked in the "controller configuration" menu
CCMD(snap45left)
{
	set_turn_aliases();
	AddCommandString("turn45left\n");
}
CCMD(snap45right)
{
	set_turn_aliases();
	AddCommandString("turn45right");
}

void Stereo3D::resetPosition() {
	if (sharedRiftHmd == NULL) return; 
	sharedRiftHmd->recenter_pose();
}

CCMD(vr_reset_position)
{
	Stereo3DMode.resetPosition();
}

// Render HUD items twice, once for each eye
// TODO - these flags don't work
static bool doBufferHud = true;


// Global shared Stereo3DMode object
Stereo3D Stereo3DMode;

static fixed_t savedPlayerViewHeight = 0;

// Delegated screen functions from v_video.h
int getStereoScreenWidth() {
	return Stereo3DMode.getScreenWidth();
}
int getStereoScreenHeight() {
	return Stereo3DMode.getScreenHeight();
}
void stereoScreenUpdate() {
	Stereo3DMode.updateScreen();
}

// length scale, to convert from meters to doom units
static float calc_mapunits_per_meter(player_t * player) {
	float vh = 41.0;
	if (player != NULL)
		vh = FIXED2FLOAT(player->mo->ViewHeight);
	return vh/(0.95 * vr_player_height_meters);
}

// Stack-scope class to temporarily adjust view position, based on positional tracking
struct ViewPositionShifter {
	ViewPositionShifter(player_t * player, FGLRenderer& renderer_param)
		: mapunits_per_meter(41.0)
		, renderer(&renderer_param)
		, saved_viewx(viewx)
		, saved_viewy(viewy)
		, saved_viewz(viewz)
	{
		mapunits_per_meter = calc_mapunits_per_meter(player);
	}

	// restore camera position after object falls out of scope
	virtual ~ViewPositionShifter() {
		setPositionFixed(saved_viewx, saved_viewy, saved_viewz);
	}

protected:
	// In player camera coordinates
	void incrementPositionFloat(float dx, float dy, float dz) {
		float xf = FIXED2FLOAT(viewx);
		float yf = FIXED2FLOAT(viewy);
		float zf = FIXED2FLOAT(viewz);

		// TODO - conversion from player to doom coordinates does not take into account roll.

		// view angle, for conversion from body to world
		float yaw = DEG2RAD( ANGLE_TO_FLOAT(viewangle) );
		float cy = cos(yaw);
		float sy = sin(yaw);

		zf += dz * mapunits_per_meter / 1.20; // doom pixel aspect correction 1.20
		xf += ( sy * dx + cy * dy) * mapunits_per_meter;
		yf += (-cy * dx + sy * dy) * mapunits_per_meter;

		setPositionFixed( FLOAT2FIXED(xf), FLOAT2FIXED(yf), FLOAT2FIXED(zf) );
	}

	// In doom world coordinates
	void setPositionFixed(fixed_t x, fixed_t y, fixed_t z) {
		viewx = x;
		viewy = y;
		viewz = z;
		renderer->SetCameraPos(viewx, viewy, viewz, viewangle);
		renderer->SetViewMatrix(false, false);
	}

private:
	fixed_t saved_viewx;
	fixed_t saved_viewy;
	fixed_t saved_viewz;
	FGLRenderer * renderer;
	float mapunits_per_meter;
};

enum EyeView {
	EYE_VIEW_LEFT,
	EYE_VIEW_RIGHT
};


// Stack-scope class to temporarily shift the camera position for stereoscopic rendering.
struct EyeViewShifter : public ViewPositionShifter
{
	// construct a new EyeViewShifter, to temporarily shift camera viewpoint
	EyeViewShifter(EyeView eyeView, player_t * player, FGLRenderer& renderer_param)
		: ViewPositionShifter(player, renderer_param)
	{
		float eyeShift = vr_ipd / 2.0;
		if (eyeView == EYE_VIEW_LEFT)
			eyeShift = -eyeShift;
		if (vr_swap)
			eyeShift = -eyeShift;
		// Account for roll angle
		float roll = renderer_param.mAngles.Roll * 3.14159/180.0;
		float cr = cos(roll);
		float sr = sin(roll);
		// Printf("%.3f\n", roll);
		/* */
		incrementPositionFloat(
			cr * eyeShift, // left-right
			0, 
			-sr * eyeShift  // up-down; sign adjusted empirically
			);
		/*	*/
	}
};


// Stack-scope class to temporarily shift the camera position for stereoscopic rendering.
struct PositionTrackingShifter : public ViewPositionShifter
{
	// construct a new EyeViewShifter, to temporarily shift camera viewpoint
	PositionTrackingShifter(RiftHmd * tracker, player_t * player, FGLRenderer& renderer_param)
		: ViewPositionShifter(player, renderer_param)
	{
		if (tracker == NULL) return;
		// TODO - calibrate to center...
		// Doom uses Z-UP convention, Rift uses Y-UP convention
		// Printf("%.3f\n", tracker->getPositionX());
		ovrPosef pose = tracker->getCurrentEyePose();

		// Convert from Rift camera coordinates to game coordinates
		// float gameYaw = renderer_param.mAngles.Yaw;
		// Printf("%6.3f, %6.3f\n", pose.Position.x, pose.Position.z);
		OVR::Quatf hmdRot(pose.Orientation);
		float hmdYaw, hmdPitch, hmdRoll;
		hmdRot.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&hmdYaw, &hmdPitch, &hmdRoll);
		OVR::Quatf yawCorrection(OVR::Vector3f(0, 1, 0), -hmdYaw); // 
		// OVR::Vector3f trans0(pose.Position);
		OVR::Vector3f trans2 = yawCorrection.Rotate(pose.Position);
		// trans2 *= 0.5; // TODO debugging
		/* */
		incrementPositionFloat(
			 trans2.x, // pose.Position.x, // LEFT_RIGHT
			-trans2.z, // pose.Position.z, // FORWARD_BACK
			 trans2.y // pose.Position.y  // UP_DOWN
		); 
		/* */
	}
};


Stereo3D::Stereo3D() 
	: mode(MONO)
{}

static HudTexture* checkHudTexture(HudTexture* hudTexture, float screenScale) {
		if (hudTexture)
			hudTexture->setScreenScale(screenScale); // BEFORE checkScreenSize
		if ( (hudTexture == NULL) || (! hudTexture->checkScreenSize(SCREENWIDTH, SCREENHEIGHT) ) ) {
			if (hudTexture)
				delete(hudTexture);
			hudTexture = new HudTexture(SCREENWIDTH, SCREENHEIGHT, screenScale);
			HudTexture::crosshairTexture = new HudTexture(SCREENWIDTH, SCREENHEIGHT, screenScale);
			hudTexture->bindToFrameBuffer();
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT);
			hudTexture->unbind();
		}
		return hudTexture;
}

static void bindAndClearHudTexture(Stereo3D& stereo3d) {
	if (doBufferHud) {
		stereo3d.bindHudTexture(true);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		// glViewport(0, 0, SCREENWIDTH, SCREENHEIGHT);
	}
}


PitchRollYaw Stereo3D::getHeadOrientation(FGLRenderer& renderer) {
	PitchRollYaw result;
	result.pitch = 0;
	result.roll = 0;
	result.yaw = 0;

	if (mode == OCULUS_RIFT) {
		ovrPosef pose = sharedRiftHmd->getCurrentEyePose();
		OVR::Quatf hmdRot(pose.Orientation);
		if (hmdRot.IsNormalized()) {
			float yaw, pitch, roll;
			hmdRot.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&yaw, &pitch, &roll);
			result.yaw = yaw;
			result.pitch = pitch;
			result.roll = -roll;
		}
	}

	return result;
}

static void blitRiftBufferToScreen() {
	// To get the buffer image, we must BLIT BEFORE submitFrame()...
	// Mirror Rift view to desktop screen
	// Must be before submitFrame()...
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(0, 0, SCREENWIDTH, SCREENHEIGHT);
	glScissor(0, 0, SCREENWIDTH, SCREENHEIGHT);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, sharedRiftHmd->getFBHandle());
	ovrSizei v = sharedRiftHmd->getViewSize();
	// TODO - use same aspect ratio, remove stuff from desktop view if necessary
	int x = 0;
	int y = 0;
	int w = v.w;
	int h = v.h;
	float riftAspect = float(v.w) / float(v.h);
	float desktopAspect = float(SCREENWIDTH) / float(SCREENHEIGHT);
	if (desktopAspect > riftAspect) {
		h = int(h*riftAspect/desktopAspect);
		y = (v.h - h) / 2;
	}
	else {
		w = int(w*desktopAspect/riftAspect);
		x = (v.w - w) / 2;
	}
	glBlitFramebuffer(x, y, w+x, h+y,
		0, 0, SCREENWIDTH, SCREENHEIGHT,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

static const float zNear = 1.0;
static const float zFar = 10000.0;

void Stereo3D::render(FGLRenderer& renderer, GL_IRECT * bounds, float fov0, float ratio0, float fovratio0, bool toscreen, sector_t * viewsector, player_t * player) 
{
	if (doBufferHud)
		LocalHudRenderer::unbind();

	// Reset pitch and roll when leaving Rift mode
	if ( (mode == OCULUS_RIFT) && ((int)mode != vr_mode) ) 
	{
		renderer.mAngles.Roll = 0;
		renderer.mAngles.Pitch = 0;

		// Blank Rift display before leaving
		sharedRiftHmd->bindToSceneFrameBufferAndUpdate();
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		sharedRiftHmd->commitFrame();
		sharedRiftHmd->submitFrame(1.0/calc_mapunits_per_meter(player));

		// Start rendering to screen, at least to start
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		sharedRiftHmd->destroy();
	}
	setMode(vr_mode);

	GLboolean supportsStereo = false;
	GLboolean supportsBuffered = false;
	// Task: manually calibrate oculusFov by slowly yawing view. 
	// If subjects approach center of view too fast, oculusFov is too small.
	// If subjects approach center of view too slowly, oculusFov is too large.
	// If subjects approach correctly , oculusFov is just right.
	// 90 is too large, 80 is too small.
	// float oculusFov = 85 * fovratio; // Hard code probably wider fov for oculus // use vr_rift_fov

	const bool doAdjustPlayerViewHeight = true; // disable/enable for testing
	if (doAdjustPlayerViewHeight) {
		if (mode == OCULUS_RIFT) {
		// if (false) {
			renderer.mCurrentFoV = vr_rift_fov; // needed for Frustum angle calculation
			// Adjust player eye height, but only in oculus rift mode...
			if (player != NULL) { // null check to avoid aliens crash
				if (savedPlayerViewHeight == 0) {
					savedPlayerViewHeight = player->mo->ViewHeight;
				}
				fixed_t testHeight = savedPlayerViewHeight + FLOAT2FIXED(vr_view_yoffset);
				if (player->mo->ViewHeight != testHeight) {
					player->mo->ViewHeight = testHeight;
					P_CalcHeight(player);
				}
			}
		} else {
			// Revert player eye height when leaving Rift mode
			if ( (savedPlayerViewHeight != 0) && (player->mo->ViewHeight != savedPlayerViewHeight) ) {
				player->mo->ViewHeight = savedPlayerViewHeight;
				savedPlayerViewHeight = 0;
				P_CalcHeight(player);
			}
		}
	}

	angle_t a1 = renderer.FrustumAngle();

	// Avoid crash when Rift is turned off (or not present?).
	// Fall back to a simpler mode if, say, rift not available
	if (mode == OCULUS_RIFT) {
		ovrResult result = sharedRiftHmd->init_graphics();
		if (! OVR_SUCCESS(result)) {
			setMode(MONO); // Revert to mono if Oculus Rift is off
			vr_mode = MONO;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}

	switch(mode) 
	{

	case MONO:
		{
			doBufferHud = false;
			setViewportFull(renderer, bounds);
			setMonoView(renderer, fov0, ratio0, fovratio0, player);
			renderer.RenderOneEye(a1, toscreen);
			renderer.EndDrawScene(viewsector);
			break;
		}

	case GREEN_MAGENTA:
		{
			doBufferHud = false;
			setViewportFull(renderer, bounds);
			{ // Local scope for color mask
				// Left eye green
				LocalScopeGLColorMask colorMask(0,1,0,1); // green
				setLeftEyeView(renderer, fov0, ratio0, fovratio0, player);
				{
					EyeViewShifter vs(EYE_VIEW_LEFT, player, renderer);
					renderer.RenderOneEye(a1, toscreen);
				}

				// Right eye magenta
				colorMask.setColorMask(1,0,1,1); // magenta
				setRightEyeView(renderer, fov0, ratio0, fovratio0, player);
				{
					EyeViewShifter vs(EYE_VIEW_RIGHT, player, renderer);
					renderer.RenderOneEye(a1, toscreen);
				}
			} // close scope to auto-revert glColorMask
			renderer.EndDrawScene(viewsector);
			break;
		}

	case RED_CYAN:
		{
			doBufferHud = false;
			setViewportFull(renderer, bounds);
			{ // Local scope for color mask
				// Left eye red
				LocalScopeGLColorMask colorMask(1,0,0,1); // red
				setLeftEyeView(renderer, fov0, ratio0, fovratio0, player);
				{
					EyeViewShifter vs(EYE_VIEW_LEFT, player, renderer);
					renderer.RenderOneEye(a1, toscreen);
				}

				// Right eye cyan
				colorMask.setColorMask(0,1,1,1); // cyan
				setRightEyeView(renderer, fov0, ratio0, fovratio0, player);
				{
					EyeViewShifter vs(EYE_VIEW_RIGHT, player, renderer);
					renderer.RenderOneEye(a1, toscreen);
				}
			} // close scope to auto-revert glColorMask
			renderer.EndDrawScene(viewsector);
			break;
		}

	case SIDE_BY_SIDE:
		{
			doBufferHud = true;
			HudTexture::hudTexture = checkHudTexture(HudTexture::hudTexture, 0.5);

			// FIRST PASS - 3D
			// Temporarily modify global variables, so HUD could draw correctly
			// each view is half width
			int oldViewwidth = viewwidth;

			int one_eye_viewport_width = oldViewwidth / 2;

			viewwidth = one_eye_viewport_width;
			// left
			setViewportLeft(renderer, bounds);
			setLeftEyeView(renderer, fov0, ratio0/2, fovratio0, player); // TODO is that fovratio?
			{
				EyeViewShifter vs(EYE_VIEW_LEFT, player, renderer);
				renderer.RenderOneEye(a1, false); // False, to not swap yet
			}
			// right
			// right view is offset to right
			int oldViewwindowx = viewwindowx;
			viewwindowx += one_eye_viewport_width;
			setViewportRight(renderer, bounds);
			setRightEyeView(renderer, fov0, ratio0/2, fovratio0, player);
			{
				EyeViewShifter vs(EYE_VIEW_RIGHT, player, renderer);
				renderer.RenderOneEye(a1, toscreen);
			}

			//
			// SECOND PASS weapon sprite
			// Weapon sprite to bottom, at expense of edges
			viewwidth = 2 * one_eye_viewport_width;
			viewwindowx = one_eye_viewport_width/2;
			//
			renderer.EndDrawScene(viewsector); // right view
			viewwindowx -= one_eye_viewport_width;
			renderer.EndDrawScene(viewsector); // left view

			blitHudTextureToScreen(2.0);

			//
			// restore global state
			viewwidth = oldViewwidth;
			viewwindowx = oldViewwindowx;

			bindAndClearHudTexture(*this);

			break;
		}

	case SIDE_BY_SIDE_SQUISHED:
		{
			doBufferHud = true;
			HudTexture::hudTexture = checkHudTexture(HudTexture::hudTexture, 0.5);

			// FIRST PASS - 3D
			// Temporarily modify global variables, so HUD could draw correctly
			// each view is half width
			int oldViewwidth = viewwidth;

			int one_eye_viewport_width = oldViewwidth / 2;

			viewwidth = one_eye_viewport_width;
			// left
			setViewportLeft(renderer, bounds);
			setLeftEyeView(renderer, fov0, ratio0, fovratio0*2, player);
			{
				EyeViewShifter vs(EYE_VIEW_LEFT, player, renderer);
				renderer.RenderOneEye(a1, toscreen);
			}
			// right
			// right view is offset to right
			int oldViewwindowx = viewwindowx;
			viewwindowx += one_eye_viewport_width;
			setViewportRight(renderer, bounds);
			setRightEyeView(renderer, fov0, ratio0, fovratio0*2, player);
			{
				EyeViewShifter vs(EYE_VIEW_RIGHT, player, renderer);
				renderer.RenderOneEye(a1, false);
			}
			//

			// SECOND PASS weapon sprite
			// viewwidth = oldViewwidth/2; // TODO - narrow aspect of weapon...
			// Ensure weapon is at bottom of screen
			viewwidth = 2 * one_eye_viewport_width;
			viewwindowx = one_eye_viewport_width/2;

			// TODO - encapsulate weapon shift for other modes
			int weaponShift = int( -vr_ipd * 0.25 * one_eye_viewport_width / (vr_weapondist * 2.0*tan(0.5*fov0)) );
			viewwindowx += weaponShift;

			renderer.EndDrawScene(viewsector); // right view
			viewwindowx -= one_eye_viewport_width;
			viewwindowx -= 2*weaponShift;
			renderer.EndDrawScene(viewsector); // left view

			blitHudTextureToScreen(2.0);

			//
			// restore global state
			viewwidth = oldViewwidth;
			viewwindowx = oldViewwindowx;

			bindAndClearHudTexture(*this);

			break;
		}

	case OCULUS_RIFT:
		{
			doBufferHud = true;
			ovrResult result = sharedRiftHmd->init_graphics();

			{
				// Activate positional tracking
				// PositionTrackingShifter positionTracker(sharedRiftHmd, player, renderer);

				setViewDirection(renderer);

				HudTexture::hudTexture = checkHudTexture(HudTexture::hudTexture, 1.0);

				sharedRiftHmd->bindToSceneFrameBufferAndUpdate();
				glClearColor(0, 0, 0, 0);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				static bool hmdEverRecentered = false;
				if (! hmdEverRecentered) {
					hmdEverRecentered = true;
					sharedRiftHmd->recenter_pose();
				}

				// FIRST PASS - 3D
				// Temporarily modify global variables, so HUD could draw correctly
				int oldScreenBlocks = screenblocks;
				screenblocks = 12; // full screen
				//
				glEnable(GL_DEPTH_TEST); // required for correct depth sorting
				glEnable(GL_STENCIL_TEST); // required for correct clipping of unhandled texture hack flats
				gl_RenderState.Set2DMode(false); // required for correct sector darkening in map mode
				// left eye view - 3D scene pass
				{
					sharedRiftHmd->setSceneEyeView(ovrEye_Left, zNear, zFar); // Left eye
					PositionTrackingShifter positionTracker(sharedRiftHmd, player, renderer);
					renderer.RenderOneEye(a1, false);
				}
				ovrPosef leftEyePose = sharedRiftHmd->getCurrentEyePose();

				// right eye view - 3D scene pass
				{
					sharedRiftHmd->setSceneEyeView(ovrEye_Right, zNear, zFar); // Right eye
					PositionTrackingShifter positionTracker(sharedRiftHmd, player, renderer);
					renderer.RenderOneEye(a1, false);
				}
				ovrPosef rightEyePose = sharedRiftHmd->getCurrentEyePose();

				// Our mode of painting screen quads for HUD, crosshair, and weapon
				// depends on whether invulnerability is on

				int hud_weap_blend1 = GL_ONE; // first ingredient of blend mode for nice background effects
				if (gl_fixedcolormap) { // invulnerability or night goggles
					// so the HUD and weapon panes won't be opaque
					hud_weap_blend1 = GL_SRC_ALPHA;
				}

				//// HUD Pass ////
				gl_RenderState.EnableAlphaTest(false);
				gl_RenderState.BlendFunc(hud_weap_blend1, GL_ONE_MINUS_SRC_ALPHA);
				gl_RenderState.Apply();

				HudTexture::hudTexture->bindRenderTexture();
				glDisable(GL_DEPTH_TEST);
				float hudPitchDegrees = -5 - 20 * vr_hud_scale / 0.6; // -25 is good for vr_hud_scale 0.6
				// note: crosshair is suppressed during hud pass?
				// left eye view - hud pass
				{
					sharedRiftHmd->setSceneEyeView(ovrEye_Left, zNear, zFar); // Left eye
					PositionTrackingShifter positionTracker(sharedRiftHmd, player, renderer);
					sharedRiftHmd->paintHudQuad(vr_hud_scale, hudPitchDegrees, 20);
				}
				// right eye view - hud pass
				{
					sharedRiftHmd->setSceneEyeView(ovrEye_Right, zNear, zFar); // Right eye
					PositionTrackingShifter positionTracker(sharedRiftHmd, player, renderer);
					sharedRiftHmd->paintHudQuad(vr_hud_scale, hudPitchDegrees, 20);
				}

				//// Crosshair Pass ////
				gl_RenderState.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				gl_RenderState.Apply();

				HudTexture::crosshairTexture->bindRenderTexture();

				// left eye view - crosshair pass
				{
					sharedRiftHmd->setSceneEyeView(ovrEye_Left, zNear, zFar); // Left eye
					PositionTrackingShifter positionTracker(sharedRiftHmd, player, renderer);
					sharedRiftHmd->paintCrosshairQuad(leftEyePose, rightEyePose, oldScreenBlocks <= 10);
				}
				// right eye view - crosshair pass
				{
					sharedRiftHmd->setSceneEyeView(ovrEye_Right, zNear, zFar); // Right eye
					PositionTrackingShifter positionTracker(sharedRiftHmd, player, renderer);
					sharedRiftHmd->paintCrosshairQuad(rightEyePose, leftEyePose, oldScreenBlocks <= 10);
				}

				/* */
				//// Weapon Pass ////
				// Temporarily adjust view window parameters, to fool weapon into drawing full size,
				// even when "screenblocks" is a smaller number
				int oldViewWindowX = viewwindowx;
				int oldViewWindowY = viewwindowy;
				int oldViewWidth = viewwidth;
				int oldViewHeight = viewheight;
				viewwindowx = 0;
				viewwindowy = 0;
				viewwidth = SCREENWIDTH;
				viewheight = SCREENHEIGHT;

				gl_RenderState.EnableAlphaTest(true);
				glDisable(GL_BLEND); // Required to get partial weapon visibility during invisible mode
				gl_RenderState.Apply();
				bindAndClearHudTexture(*this);
				renderer.EndDrawSceneSprites(viewsector); // paint weapon
				HudTexture::hudTexture->unbind();
				glEnable(GL_BLEND);

				sharedRiftHmd->bindToSceneFrameBuffer();
				HudTexture::hudTexture->bindRenderTexture();

				gl_RenderState.EnableAlphaTest(false);
				gl_RenderState.BlendFunc(hud_weap_blend1, GL_ONE_MINUS_SRC_ALPHA);
				gl_RenderState.Apply(); // good - suit no longer obscures weapon; implicitly enables GL_TEXTURE_2D

				// left eye view - weapon pass
				{
					sharedRiftHmd->setSceneEyeView(ovrEye_Left, zNear, zFar); // Left eye
					PositionTrackingShifter positionTracker(sharedRiftHmd, player, renderer);
					sharedRiftHmd->paintWeaponQuad(leftEyePose, rightEyePose, vr_weapondist, vr_weapon_height);
				}
				// right eye view - weapon pass
				{
					sharedRiftHmd->setSceneEyeView(ovrEye_Right, zNear, zFar); // Right eye
					PositionTrackingShifter positionTracker(sharedRiftHmd, player, renderer);
					sharedRiftHmd->paintWeaponQuad(rightEyePose, leftEyePose, vr_weapondist, vr_weapon_height);
				}

				//// Blend Effects Pass
				{ //  separate pass for full screen effects like radiation suit
					bindAndClearHudTexture(*this);
					renderer.EndDrawSceneBlend(viewsector); // paint suit effects etc.
					HudTexture::hudTexture->unbind();

					sharedRiftHmd->bindToSceneFrameBuffer();
					HudTexture::hudTexture->bindRenderTexture();

					gl_RenderState.EnableAlphaTest(false);
					gl_RenderState.BlendFunc(hud_weap_blend1, GL_ONE_MINUS_SRC_ALPHA);
					gl_RenderState.Apply();
					{
						sharedRiftHmd->setSceneEyeView(ovrEye_Left, zNear, zFar); // Left eye
						sharedRiftHmd->paintBlendQuad();
					}
					{
						sharedRiftHmd->setSceneEyeView(ovrEye_Right, zNear, zFar); // Right eye
						sharedRiftHmd->paintBlendQuad();
					}
				}


				sharedRiftHmd->commitFrame();

				// glEnable(GL_BLEND);
				glBindTexture(GL_TEXTURE_2D, 0);
				// Restore previous values
				viewwindowx = oldViewWindowX;
				viewwindowy = oldViewWindowY;
				viewwidth = oldViewWidth;
				viewheight = oldViewHeight;

				static bool swapThisFrame = true;
				// swapThisFrame = ! swapThisFrame; // Only mirror every other frame
				if (swapThisFrame)
					blitRiftBufferToScreen();

				sharedRiftHmd->submitFrame(1.0/calc_mapunits_per_meter(player));

				if (swapThisFrame) // causes yaw judder with OVR SDK 1.3
				{
					// ... but to keep frame rate, we must Swap() AFTER submitFrame
					All.Unclock();
					static_cast<OpenGLFrameBuffer*>(screen)->Swap();
					All.Clock();				
				}

				// Clear crosshair
				HudTexture::crosshairTexture->bindToFrameBuffer();
				glViewport(0, 0, SCREENWIDTH, SCREENHEIGHT);
				glScissor(0, 0, SCREENWIDTH, SCREENHEIGHT);
				glClearColor(0, 0, 0, 0);
				glClear(GL_COLOR_BUFFER_BIT);

				// Clear HUD
				HudTexture::hudTexture->bindToFrameBuffer();

				glViewport(0, 0, SCREENWIDTH, SCREENHEIGHT);
				glScissor(0, 0, SCREENWIDTH, SCREENHEIGHT);
				glClearColor(0,0,0,0);
				glClear(GL_COLOR_BUFFER_BIT);

				screenblocks = oldScreenBlocks;
			}

			break;
		}

	case LEFT_EYE_VIEW:
		{
			doBufferHud = false;
			setViewportFull(renderer, bounds);
			setLeftEyeView(renderer, fov0, ratio0, fovratio0, player);
			{
				EyeViewShifter vs(EYE_VIEW_LEFT, player, renderer);
				renderer.RenderOneEye(a1, toscreen);
			}
			renderer.EndDrawScene(viewsector);
			break;
		}

	case RIGHT_EYE_VIEW:
		{
			doBufferHud = false;
			setViewportFull(renderer, bounds);
			setRightEyeView(renderer, fov0, ratio0, fovratio0, player);
			{
				EyeViewShifter vs(EYE_VIEW_RIGHT, player, renderer);
				renderer.RenderOneEye(a1, toscreen);
			}
			renderer.EndDrawScene(viewsector);
			break;
		}

	case QUAD_BUFFERED:
		{
			doBufferHud = false;
			setViewportFull(renderer, bounds);
			glGetBooleanv(GL_STEREO, &supportsStereo);
			glGetBooleanv(GL_DOUBLEBUFFER, &supportsBuffered);
			if (supportsStereo && supportsBuffered && toscreen)
			{ 
				// Right first this time, so more generic GL_BACK_LEFT will remain for other modes
				glDrawBuffer(GL_BACK_RIGHT);
				setRightEyeView(renderer, fov0, ratio0, fovratio0, player);
				{
					EyeViewShifter vs(EYE_VIEW_RIGHT, player, renderer);
					renderer.RenderOneEye(a1, toscreen);
				}
				// Left
				glDrawBuffer(GL_BACK_LEFT);
				setLeftEyeView(renderer, fov0, ratio0, fovratio0, player);
				{
					EyeViewShifter vs(EYE_VIEW_LEFT, player, renderer);
					renderer.RenderOneEye(a1, toscreen);
				}
				// Want HUD in both views
				glDrawBuffer(GL_BACK);
			} else { // mono view, in case hardware stereo is not supported
				setMonoView(renderer, fov0, ratio0, fovratio0, player);
				renderer.RenderOneEye(a1, toscreen);			
			}
			renderer.EndDrawScene(viewsector);
			break;
		}

	default:
		{
			doBufferHud = false;
			setViewportFull(renderer, bounds);
			setMonoView(renderer, fov0, ratio0, fovratio0, player);
			renderer.RenderOneEye(a1, toscreen);			
			renderer.EndDrawScene(viewsector);
			break;
		}

	}
}

void Stereo3D::bindHudTexture(bool doUse)
{
	HudTexture * ht = HudTexture::hudTexture;
	if (ht == NULL)
		return;
	if (! doUse) {
		ht->unbind(); // restore drawing to real screen
	}
	if (! doBufferHud)
		return;
	if (doUse) {
		ht->bindToFrameBuffer();
		glViewport(0, 0, ht->getWidth(), ht->getHeight());
		glScissor(0, 0, ht->getWidth(), ht->getHeight());
	}
	else {
		ht->unbind(); // restore drawing to real screen
		glViewport(0, 0, screen->GetWidth(), screen->GetHeight());
		glScissor(0, 0, screen->GetWidth(), screen->GetHeight());
	}
}

// Here is where to update Rift in non-level situations
void Stereo3D::updateScreen() {
	gamestate_t x = gamestate;
	// Unbind texture before update, so Fraps could work
	bool htWasBound = false;
	HudTexture * ht = HudTexture::hudTexture;
	if (ht && ht->isBound()) {
		htWasBound = true;
		ht->unbind();
		// blitHudTextureToScreen(); // causes double hud in rift
	}
	// Avoid crash when Rift has not been powered on yet
	static bool wasRiftMode = false;
	if ( (vr_mode == OCULUS_RIFT) ) {
		ovrResult result = sharedRiftHmd->init_graphics();
		if (OVR_SUCCESS(result)) {
			wasRiftMode = true;
		}
		else {
			// setMode(MONO); // Revert to mono if Oculus Rift is off
			vr_mode = MONO;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}
	// Make sure display is usable after leaving Rift mode
	else if (wasRiftMode) {
		// Blank Rift display before leaving
		sharedRiftHmd->bindToSceneFrameBufferAndUpdate();
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		sharedRiftHmd->commitFrame();
		sharedRiftHmd->submitFrame(1.0/41.0);
		// Start rendering to screen, at least for the moment
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		wasRiftMode = false;
		htWasBound = false;
	}
	// Special handling of intermission screen for Oculus SDK warping
	if ( (vr_mode == OCULUS_RIFT) ) 
	{
		// TODO - what about fraps?
		// update Rift during "other" modes, such as title screen
		if (gamestate != GS_LEVEL) 
		{
			sharedRiftHmd->bindToSceneFrameBufferAndUpdate();
			// Nice dark red universe
			glClearColor(0.15, 0.03, 0.03, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			//// HUD Pass ////
			HudTexture::hudTexture = checkHudTexture(HudTexture::hudTexture, 1.0);
			HudTexture::hudTexture->bindRenderTexture();
			glEnable(GL_TEXTURE_2D);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
			// glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			// glAlphaFunc(GL_GREATER, 0.4); // 0.2 -> 0.3 causes console background to show
			// TODO suppress crosshair during hud pass?
			// left eye view - hud pass
			{
				sharedRiftHmd->setSceneEyeView(ovrEye_Left, zNear, zFar); // Left eye
				// PositionTrackingShifter positionTracker(sharedRiftHmd, player, renderer);
				sharedRiftHmd->paintHudQuad(vr_hud_scale, 0, 50);
			}
			// right eye view - hud pass
			{
				sharedRiftHmd->setSceneEyeView(ovrEye_Right, zNear, zFar); // Right eye
				// PositionTrackingShifter positionTracker(sharedRiftHmd, player, renderer);
				sharedRiftHmd->paintHudQuad(vr_hud_scale, 0, 50);
			}

			sharedRiftHmd->commitFrame();
			blitRiftBufferToScreen();
			sharedRiftHmd->submitFrame(1.0/41.0);

			if (true) {
				// ... but to keep frame rate, we must Swap() AFTER submitFrame
				All.Unclock();
				static_cast<OpenGLFrameBuffer*>(screen)->Swap();
				All.Clock();				
			}

			// Clear HUD
			HudTexture::hudTexture->bindToFrameBuffer();
			glViewport(0, 0, SCREENWIDTH, SCREENHEIGHT);
			glScissor(0, 0, SCREENWIDTH, SCREENHEIGHT);
			glClearColor(0.1 , 0.1, 0.1, 0.0); // Gray default
			glClear(GL_COLOR_BUFFER_BIT);
			glEnable(GL_BLEND);
		}


	} else {
		screen->Update();
	}
	if (htWasBound)
		ht->bindToFrameBuffer();
}

int Stereo3D::getScreenWidth() {
	return screen->GetWidth();
}
int Stereo3D::getScreenHeight() {
	return screen->GetHeight();
}

void Stereo3D::blitHudTextureToScreen(float yScale) {
	glEnable(GL_TEXTURE_2D);
	if (! doBufferHud)
		return;

	// Compute viewport coordinates
	float h = HudTexture::hudTexture->getHeight() * yScale;
	float w = HudTexture::hudTexture->getWidth();
	float x = (SCREENWIDTH/2-w)*0.5;
	float hudOffsetY = 0.00 * h; // nudge crosshair up
	int hudOffsetX = 0; // kludge to set apparent hud distance

	if (mode == OCULUS_RIFT) {
		// First pass blit unwarped hudTexture into oculusTexture, in two places
		hudOffsetY -= 0.005 * SCREENHEIGHT; // reverse effect of oculus head raising.
		hudOffsetX = (int)(0.004*SCREENWIDTH/2); // kludge to set hud distance
		if (vr_swap)
			hudOffsetX = -hudOffsetX;
		if (screenblocks <= 10)
			hudOffsetY -= 0.080 * h; // lower crosshair when status bar is on
	}

	// Left side
	float y = (SCREENHEIGHT-h)*0.5 + hudOffsetY; // offset to move cross hair up to correct spot
	glViewport(x+hudOffsetX, y, w, h); // Not infinity, but not as close as the weapon.
	HudTexture::hudTexture->renderToScreen();

	// Right side
	x += SCREENWIDTH/2;
	glViewport(x-hudOffsetX, y, w, h);
	HudTexture::hudTexture->renderToScreen();

	if (mode == OCULUS_RIFT) {
	}
}

void Stereo3D::setMode(int m) {
	mode = static_cast<Mode>(m);
};

void Stereo3D::setMonoView(FGLRenderer& renderer, float fov, float ratio, float fovratio, player_t * player) {
	renderer.SetProjection(fov, ratio, fovratio, 0);
}

void Stereo3D::setLeftEyeView(FGLRenderer& renderer, float fov, float ratio, float fovratio, player_t * player, bool frustumShift) {
	renderer.SetProjection(fov, ratio, fovratio, vr_swap ? +vr_ipd/2 : -vr_ipd/2, frustumShift);
}

void Stereo3D::setRightEyeView(FGLRenderer& renderer, float fov, float ratio, float fovratio, player_t * player, bool frustumShift) {
	renderer.SetProjection(fov, ratio, fovratio, vr_swap ? -vr_ipd/2 : +vr_ipd/2, frustumShift);
}

bool Stereo3D::hasHeadTracking() const {
	if (! (mode == OCULUS_RIFT) )
		return false;
	return true;
}

void Stereo3D::setViewDirection(FGLRenderer& renderer) {
	// Set HMD angle parameters for NEXT frame
	static float previousYaw = 0;
	static bool havePreviousYaw = false;
	if (mode == OCULUS_RIFT) {
		PitchRollYaw prw = getHeadOrientation(renderer);
		if (! havePreviousYaw) {
			previousYaw = prw.yaw;
			havePreviousYaw = true;
		}
		double dYaw = prw.yaw - previousYaw;
		G_AddViewAngle(-32768.0*dYaw/3.14159); // determined empirically
		previousYaw = prw.yaw;

		// Pitch
		int pitch = -32768/3.14159*prw.pitch;
		int dPitch = (pitch - viewpitch/65536); // empirical
		G_AddViewPitch(-dPitch);

		// Roll can be local, because it doesn't affect gameplay.
		renderer.mAngles.Roll = prw.roll * 180.0 / 3.14159;
	}
}

// Normal full screen viewport
void Stereo3D::setViewportFull(FGLRenderer& renderer, GL_IRECT * bounds) {
	renderer.SetViewport(bounds);
}

// Left half of screen
void Stereo3D::setViewportLeft(FGLRenderer& renderer, GL_IRECT * bounds) {
	if (bounds) {
		GL_IRECT leftBounds;
		leftBounds.width = bounds->width / 2;
		leftBounds.height = bounds->height;
		leftBounds.left = bounds->left;
		leftBounds.top = bounds->top;
		renderer.SetViewport(&leftBounds);
	}
	else {
		renderer.SetViewport(bounds);
	}
}

// Right half of screen
void Stereo3D::setViewportRight(FGLRenderer& renderer, GL_IRECT * bounds) {
	if (bounds) {
		GL_IRECT rightBounds;
		rightBounds.width = bounds->width / 2;
		rightBounds.height = bounds->height;
		rightBounds.left = bounds->left + rightBounds.width;
		rightBounds.top = bounds->top;
		renderer.SetViewport(&rightBounds);
	}
	else {
		renderer.SetViewport(bounds);
	}
}

/* static */ void LocalHudRenderer::bind()
{
	Stereo3DMode.bindHudTexture(true);
}

/* static */ void LocalHudRenderer::unbind()
{
	Stereo3DMode.bindHudTexture(false);
}

LocalHudRenderer::LocalHudRenderer()
{
	bind();
}

LocalHudRenderer::~LocalHudRenderer()
{
	unbind();
}

