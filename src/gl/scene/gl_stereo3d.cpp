#include "v_video.h"
#include "gl/system/gl_system.h"
#include "gl/system/gl_cvars.h"
#include "gl/system/gl_framebuffer.h"
#include "gl/scene/gl_stereo3d.h"
#include "gl/renderer/gl_renderer.h"
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

extern void P_CalcHeight (player_t *player);

#define RIFT_HUDSCALE 0.40

EXTERN_CVAR(Bool, gl_draw_sync)
EXTERN_CVAR(Float, vr_screendist)
//
CVAR(Int, vr_mode, 0, CVAR_GLOBALCONFIG)
CVAR(Bool, vr_swap, false, CVAR_GLOBALCONFIG)
// intraocular distance in meters
CVAR(Float, vr_ipd, 0.062f, CVAR_ARCHIVE|CVAR_GLOBALCONFIG) // METERS
CVAR(Float, vr_rift_fov, 115.0f, CVAR_ARCHIVE|CVAR_GLOBALCONFIG) // DEGREES
CVAR(Float, vr_view_yoffset, 0.0, 0) // MAP UNITS
// Especially Oculus Rift VR geometry depends on exact mapping between doom map units and real world.
// Supposed to be 32 units per meter, according to http://doom.wikia.com/wiki/Map_unit
// But ceilings and floors look too close at that scale.
CVAR(Float, vr_player_height_meters, 1.7f, CVAR_ARCHIVE|CVAR_GLOBALCONFIG) // Used for stereo 3D
CVAR(Float, vr_rift_aspect, 640.0/800.0, CVAR_ARCHIVE|CVAR_GLOBALCONFIG) // Used for stereo 3D
CVAR(Float, vr_weapon_height, 0.0, CVAR_ARCHIVE|CVAR_GLOBALCONFIG) // Used for oculus rift
CVAR(Float, vr_weapondist, 0.6, CVAR_ARCHIVE|CVAR_GLOBALCONFIG) // METERS


// Render HUD items twice, once for each eye
// TODO - these flags don't work
const bool doBufferHud = true;
const bool doBufferOculus = true;


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

enum EyeView {
	EYE_VIEW_LEFT,
	EYE_VIEW_RIGHT
};

// Stack-scope class to temporarily shift the camera position for stereoscopic rendering.
struct ViewShifter {
	// construct a new ViewShifter, to temporarily shift camera viewpoint
	ViewShifter(EyeView eyeView, player_t * player, FGLRenderer& renderer_param) {
		renderer = &renderer_param;
		saved_viewx = viewx;
		saved_viewy = viewy;

		float xf = FIXED2FLOAT(viewx);
		float yf = FIXED2FLOAT(viewy);
		float yaw = DEG2RAD( ANGLE_TO_FLOAT(viewangle) );

		float eyeShift = vr_ipd / 2.0;
		if (eyeView == EYE_VIEW_LEFT)
			eyeShift = -eyeShift;
		float vh = 41.0;
		if (player != NULL)
			vh = FIXED2FLOAT(player->mo->ViewHeight);
		float mapunits_per_meter = vh/(0.95 * vr_player_height_meters);
		float eyeShift_mapunits = eyeShift * mapunits_per_meter;

		xf += sin(yaw) * eyeShift_mapunits;
		yf -= cos(yaw) * eyeShift_mapunits;
		// Printf("eyeShift_mapunits (ViewShifter) = %.1f\n", eyeShift_mapunits);

		viewx = FLOAT2FIXED(xf);
		viewy = FLOAT2FIXED(yf);
		renderer->SetCameraPos(viewx, viewy, viewz, viewangle);
		renderer->SetViewMatrix(false, false);
	}

	// restore camera position after object falls out of scope
	~ViewShifter() {
		viewx = saved_viewx;
		viewy = saved_viewy;
		renderer->SetCameraPos(viewx, viewy, viewz, viewangle);
		renderer->SetViewMatrix(false, false);
	}

private:
	fixed_t saved_viewx;
	fixed_t saved_viewy;
	FGLRenderer * renderer;
};

Stereo3D::Stereo3D() 
	: mode(MONO)
	, oculusTexture(NULL)
	, hudTexture(NULL)
	, oculusTracker(NULL)
	, adaptScreenSize(false)
{}

void Stereo3D::render(FGLRenderer& renderer, GL_IRECT * bounds, float fov0, float ratio0, float fovratio0, bool toscreen, sector_t * viewsector, player_t * player) 
{
	if (doBufferHud)
		LocalHudRenderer::unbind();

	// Reset pitch and roll when leaving Rift mode
	if ( (mode == OCULUS_RIFT) && ((int)mode != vr_mode) ) {
		renderer.mAngles.Roll = 0;
		renderer.mAngles.Pitch = 0;
	}
	setMode(vr_mode);

	// Restore actual screen, instead of offscreen single-eye buffer,
	// in case we just exited Rift mode.
	adaptScreenSize = false;

	GLboolean supportsStereo = false;
	GLboolean supportsBuffered = false;
	// Task: manually calibrate oculusFov by slowly yawing view. 
	// If subjects approach center of view too fast, oculusFov is too small.
	// If subjects approach center of view too slowly, oculusFov is too large.
	// If subjects approach correctly , oculusFov is just right.
	// 90 is too large, 80 is too small.
	// float oculusFov = 85 * fovratio; // Hard code probably wider fov for oculus // use vr_rift_fov

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

	angle_t a1 = renderer.FrustumAngle();

	switch(mode) 
	{

	case MONO:
		setViewportFull(renderer, bounds);
		setMonoView(renderer, fov0, ratio0, fovratio0, player);
		renderer.RenderOneEye(a1, toscreen, true);
		renderer.EndDrawScene(viewsector);
		break;

	case GREEN_MAGENTA:
		setViewportFull(renderer, bounds);
		{ // Local scope for color mask
			// Left eye green
			LocalScopeGLColorMask colorMask(0,1,0,1); // green
			setLeftEyeView(renderer, fov0, ratio0, fovratio0, player);
			{
				ViewShifter vs(EYE_VIEW_LEFT, player, renderer);
				renderer.RenderOneEye(a1, toscreen, false);
			}

			// Right eye magenta
			colorMask.setColorMask(1,0,1,1); // magenta
			setRightEyeView(renderer, fov0, ratio0, fovratio0, player);
			{
				ViewShifter vs(EYE_VIEW_RIGHT, player, renderer);
				renderer.RenderOneEye(a1, toscreen, true);
			}
		} // close scope to auto-revert glColorMask
		renderer.EndDrawScene(viewsector);
		break;

	case RED_CYAN:
		setViewportFull(renderer, bounds);
		{ // Local scope for color mask
			// Left eye red
			LocalScopeGLColorMask colorMask(1,0,0,1); // red
			setLeftEyeView(renderer, fov0, ratio0, fovratio0, player);
			{
				ViewShifter vs(EYE_VIEW_LEFT, player, renderer);
				renderer.RenderOneEye(a1, toscreen, false);
			}

			// Right eye cyan
			colorMask.setColorMask(0,1,1,1); // cyan
			setRightEyeView(renderer, fov0, ratio0, fovratio0, player);
			{
				ViewShifter vs(EYE_VIEW_RIGHT, player, renderer);
				renderer.RenderOneEye(a1, toscreen, true);
			}
		} // close scope to auto-revert glColorMask
		renderer.EndDrawScene(viewsector);
		break;

	case SIDE_BY_SIDE:
		{
			// FIRST PASS - 3D
			// Temporarily modify global variables, so HUD could draw correctly
			// each view is half width
			int oldViewwidth = viewwidth;
			viewwidth = viewwidth/2;
			// left
			setViewportLeft(renderer, bounds);
			setLeftEyeView(renderer, fov0, ratio0/2, fovratio0, player); // TODO is that fovratio?
			{
				ViewShifter vs(EYE_VIEW_LEFT, player, renderer);
				renderer.RenderOneEye(a1, false, false); // False, to not swap yet
			}
			// right
			// right view is offset to right
			int oldViewwindowx = viewwindowx;
			viewwindowx += viewwidth;
			setViewportRight(renderer, bounds);
			setRightEyeView(renderer, fov0, ratio0/2, fovratio0, player);
			{
				ViewShifter vs(EYE_VIEW_RIGHT, player, renderer);
				renderer.RenderOneEye(a1, toscreen, true);
			}
			//
			// SECOND PASS weapon sprite
			renderer.EndDrawScene(viewsector); // right view
			viewwindowx -= viewwidth;
			renderer.EndDrawScene(viewsector); // left view
			//
			// restore global state
			viewwidth = oldViewwidth;
			viewwindowx = oldViewwindowx;
			break;
		}

	case SIDE_BY_SIDE_SQUISHED:
		{
			// FIRST PASS - 3D
			// Temporarily modify global variables, so HUD could draw correctly
			// each view is half width
			int oldViewwidth = viewwidth;
			viewwidth = viewwidth/2;
			// left
			setViewportLeft(renderer, bounds);
			setLeftEyeView(renderer, fov0, ratio0, fovratio0*2, player);
			{
				ViewShifter vs(EYE_VIEW_LEFT, player, renderer);
				renderer.RenderOneEye(a1, toscreen, false);
			}
			// right
			// right view is offset to right
			int oldViewwindowx = viewwindowx;
			viewwindowx += viewwidth;
			setViewportRight(renderer, bounds);
			setRightEyeView(renderer, fov0, ratio0, fovratio0*2, player);
			{
				ViewShifter vs(EYE_VIEW_RIGHT, player, renderer);
				renderer.RenderOneEye(a1, false, true);
			}
			//
			// SECOND PASS weapon sprite
			viewwidth = oldViewwidth/2; // TODO - narrow aspect of weapon...

			// TODO - encapsulate weapon shift for other modes
			int weaponShift = int( -vr_ipd * 0.25 * viewwidth / (vr_weapondist * 2.0*tan(0.5*fov0)) );
			viewwindowx += weaponShift;

			renderer.EndDrawScene(viewsector); // right view
			viewwindowx -= oldViewwidth/2;
			viewwindowx -= 2*weaponShift;
			renderer.EndDrawScene(viewsector); // left view
			//
			// restore global state
			viewwidth = oldViewwidth;
			viewwindowx = oldViewwindowx;
			break;
		}

	case OCULUS_RIFT:
		{
			if ( (oculusTexture == NULL) || (! oculusTexture->checkSize(SCREENWIDTH, SCREENHEIGHT)) ) {
				if (oculusTexture)
					delete(oculusTexture);
				oculusTexture = new OculusTexture(SCREENWIDTH, SCREENHEIGHT);
			}
			if ( (hudTexture == NULL) || (! hudTexture->checkSize(SCREENWIDTH/2, SCREENHEIGHT)) ) {
				if (hudTexture)
					delete(hudTexture);
				hudTexture = new HudTexture(SCREENWIDTH/2, SCREENHEIGHT);
				hudTexture->bindToFrameBuffer();
				glClearColor(0, 0, 0, 0);
				glClear(GL_COLOR_BUFFER_BIT);
				hudTexture->unbind();
			}
			// Render unwarped image to offscreen frame buffer
			if (doBufferOculus) {
				oculusTexture->bindToFrameBuffer();
			}
			// FIRST PASS - 3D
			// Temporarily modify global variables, so HUD could draw correctly
			// each view is half width
			int oldViewwidth = viewwidth;
			viewwidth = viewwidth/2;
			int oldScreenBlocks = screenblocks;
			screenblocks = 12; // full screen
			//
			// TODO correct geometry for oculus
			//
			float ratio = 1.20 * vr_rift_aspect;
			float fovy = 2.0*atan(tan(0.5*vr_rift_fov*3.14159/180.0)/ratio) * 180.0/3.14159;
			float fovratio = vr_rift_fov/fovy;
			//
			// left
			GL_IRECT riftBounds; // Always use full screen with Oculus Rift
			riftBounds.width = SCREENWIDTH;
			riftBounds.height = SCREENHEIGHT;
			riftBounds.left = 0;
			riftBounds.top = 0;
			setViewportLeft(renderer, &riftBounds);
			setLeftEyeView(renderer, vr_rift_fov, ratio, fovratio, player, false);
			glEnable(GL_DEPTH_TEST);
			{
				ViewShifter vs(EYE_VIEW_LEFT, player, renderer);
				renderer.RenderOneEye(a1, false, false);
			}
			// right
			// right view is offset to right
			int oldViewwindowx = viewwindowx;
			viewwindowx += viewwidth;
			setViewportRight(renderer, &riftBounds);
			setRightEyeView(renderer, vr_rift_fov, ratio, fovratio, player, false);
			{
				ViewShifter vs(EYE_VIEW_RIGHT, player, renderer);
				renderer.RenderOneEye(a1, false, true);
			}

			// Second pass sprites (especially weapon)
			int oldViewwindowy = viewwindowy;
			const bool showSprites = true;
			if (showSprites) {
				// SECOND PASS weapon sprite
				glEnable(GL_TEXTURE_2D);
				screenblocks = 12;
				float fullWidth = SCREENWIDTH / 2.0;
				viewwidth = RIFT_HUDSCALE * fullWidth;
				float left = (1.0 - RIFT_HUDSCALE) * fullWidth * 0.5; // left edge of scaled viewport
				// TODO Sprite needs some offset to appear at correct distance, rather than at infinity.
				int spriteOffsetX = (int)(0.021*fullWidth); // kludge to set weapon distance
				viewwindowx = left + fullWidth - spriteOffsetX;
				int spriteOffsetY = (int)(-0.01 * vr_weapon_height * viewheight); // nudge gun up/down
				// Counteract effect of status bar on weapon position
				if (oldScreenBlocks <= 10) { // lower weapon in status mode
					spriteOffsetY += 0.227 * viewwidth; // empirical - lines up brutal doom down sight in 1920x1080 Rift mode
				}
				viewwindowy += spriteOffsetY;
				renderer.EndDrawScene(viewsector); // right view
				setViewportLeft(renderer, &riftBounds);
				viewwindowx = left + spriteOffsetX;
				renderer.EndDrawScene(viewsector); // left view
			}

			// Third pass HUD
			if (doBufferHud) {
				screenblocks = max(oldScreenBlocks, 10); // Don't vignette main 3D view
				// Draw HUD again, to avoid flashing? - and render to screen
				blitHudTextureToScreen(true); // HUD pass now occurs in main doom loop! Since I delegated screen->Update to stereo3d.updateScreen().
			}

			//
			// restore global state
			viewwidth = oldViewwidth;
			viewwindowx = oldViewwindowx;
			viewwindowy = oldViewwindowy;
			// Update orientation for NEXT frame, after expensive render has occurred this frame
			setViewDirection(renderer);
			// Set up 2D rendering to write to our hud renderbuffer
			if (doBufferHud) {
				bindHudTexture(true);
				glClearColor(0, 0, 0, 0);
				glClear(GL_COLOR_BUFFER_BIT);
				bindHudTexture(false);
				LocalHudRenderer::bind();
			}
			break;
		}

	case LEFT_EYE_VIEW:
		setViewportFull(renderer, bounds);
		setLeftEyeView(renderer, fov0, ratio0, fovratio0, player);
		{
			ViewShifter vs(EYE_VIEW_LEFT, player, renderer);
			renderer.RenderOneEye(a1, toscreen, true);
		}
		renderer.EndDrawScene(viewsector);
		break;

	case RIGHT_EYE_VIEW:
		setViewportFull(renderer, bounds);
		setRightEyeView(renderer, fov0, ratio0, fovratio0, player);
		{
			ViewShifter vs(EYE_VIEW_RIGHT, player, renderer);
			renderer.RenderOneEye(a1, toscreen, true);
		}
		renderer.EndDrawScene(viewsector);
		break;

	case QUAD_BUFFERED:
		setViewportFull(renderer, bounds);
		glGetBooleanv(GL_STEREO, &supportsStereo);
		glGetBooleanv(GL_DOUBLEBUFFER, &supportsBuffered);
		if (supportsStereo && supportsBuffered && toscreen)
		{ 
			// Right first this time, so more generic GL_BACK_LEFT will remain for other modes
			glDrawBuffer(GL_BACK_RIGHT);
			setRightEyeView(renderer, fov0, ratio0, fovratio0, player);
			{
				ViewShifter vs(EYE_VIEW_RIGHT, player, renderer);
				renderer.RenderOneEye(a1, toscreen, false);
			}
			// Left
			glDrawBuffer(GL_BACK_LEFT);
			setLeftEyeView(renderer, fov0, ratio0, fovratio0, player);
			{
				ViewShifter vs(EYE_VIEW_LEFT, player, renderer);
				renderer.RenderOneEye(a1, toscreen, true);
			}
			// Want HUD in both views
			glDrawBuffer(GL_BACK);
		} else { // mono view, in case hardware stereo is not supported
			setMonoView(renderer, fov0, ratio0, fovratio0, player);
			renderer.RenderOneEye(a1, toscreen, true);			
		}
		renderer.EndDrawScene(viewsector);
		break;

	default:
		setViewportFull(renderer, bounds);
		setMonoView(renderer, fov0, ratio0, fovratio0, player);
		renderer.RenderOneEye(a1, toscreen, true);			
		renderer.EndDrawScene(viewsector);
		break;

	}
}

void Stereo3D::bindHudTexture(bool doUse)
{
	HudTexture * ht = Stereo3DMode.hudTexture;
	if (ht == NULL)
		return;
	if (! doUse)
		ht->unbind(); // restore drawing to real screen
	if (vr_mode != OCULUS_RIFT)
		return;
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

void Stereo3D::updateScreen() {
	// Unbind texture before update, so Fraps could work
	bool htWasBound = false;
	HudTexture * ht = Stereo3DMode.hudTexture;
	if (ht && ht->isBound()) {
		htWasBound = true;
		ht->unbind();
	}
	screen->Update();
	if (htWasBound)
		ht->bindToFrameBuffer();
	if (vr_mode != OCULUS_RIFT)
		return;
	if (ht == NULL)
		return;
	if (ht->isBound()) {
		bindHudTexture(false);
		blitHudTextureToScreen();
		bindHudTexture(true);
	}
	else {
		if (doBufferHud)
			blitHudTextureToScreen();
		glViewport(0, 0, screen->GetWidth(), screen->GetHeight());
	}
}

int Stereo3D::getScreenWidth() {
	if (adaptScreenSize && hudTexture && hudTexture->isBound())
		return hudTexture->getWidth();
	return screen->GetWidth();
}
int Stereo3D::getScreenHeight() {
	if (adaptScreenSize && hudTexture && hudTexture->isBound())
		return hudTexture->getHeight();
	return screen->GetHeight();
}

void Stereo3D::blitHudTextureToScreen(bool toscreen) {
	// if (!showHud) return;
	glEnable(GL_TEXTURE_2D);
	if (mode == OCULUS_RIFT) {
		bool useOculusTexture = doBufferOculus;
		if (! oculusTexture)
			useOculusTexture = false;
		// First pass blit unwarped hudTexture into oculusTexture, in two places
		if (useOculusTexture)
			oculusTexture->bindToFrameBuffer();
		// Compute viewport coordinates
		float h = SCREENHEIGHT * RIFT_HUDSCALE * 1.00 / 2.0; // 1.20 pixel aspect is handled in view matrix TODO is it?
		float w = SCREENWIDTH/2 * RIFT_HUDSCALE;
		float x = (SCREENWIDTH/2-w)*0.5;
		float hudOffsetY = 0.01 * h; // nudge crosshair up
		hudOffsetY -= 0.005 * SCREENHEIGHT; // reverse effect of oculus head raising.
		if (screenblocks <= 10)
			hudOffsetY -= 0.080 * h; // lower crosshair when status bar is on
		float y = (SCREENHEIGHT-h)*0.5 + hudOffsetY; // offset to move cross hair up to correct spot
		int hudOffsetX = (int)(0.004*SCREENWIDTH/2); // kludge to set hud distance
		glViewport(x+hudOffsetX, y, w, h); // Not infinity, but not as close as the weapon.
		if (useOculusTexture || toscreen)
			hudTexture->renderToScreen();
		x += SCREENWIDTH/2;
		glViewport(x-hudOffsetX, y, w, h);
		if (useOculusTexture || toscreen)
			hudTexture->renderToScreen();
		// Second pass blit warped oculusTexture to screen
		if (oculusTexture && toscreen) {
			oculusTexture->unbind();
			glViewport(0, 0, SCREENWIDTH, SCREENHEIGHT);
			oculusTexture->renderToScreen();
			if (!gl_draw_sync && toscreen) {
			// if (gamestate != GS_LEVEL) { // TODO avoids flash by swapping at beginning of 3D render
				All.Unclock();
				static_cast<OpenGLFrameBuffer*>(screen)->Swap();
				All.Clock();
			}
		}
	}
	else { // TODO what else?
		if (toscreen) {
			glViewport(0, 0, SCREENWIDTH, SCREENHEIGHT);
			hudTexture->renderToScreen();
			// if (gamestate != GS_LEVEL) {
				static_cast<OpenGLFrameBuffer*>(screen)->Swap();
			// }
		}
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
	if (oculusTracker == 0)
		return false;
	if (! oculusTracker->isGood())
		return false;
	return true;
}

PitchRollYaw Stereo3D::getHeadOrientation(FGLRenderer& renderer) {
	PitchRollYaw result;

	result.pitch = renderer.mAngles.Pitch;
	result.roll = renderer.mAngles.Roll;
	result.yaw = renderer.mAngles.Yaw;

	if (mode == OCULUS_RIFT) {
		if (oculusTracker == NULL) {
			oculusTracker = new OculusTracker();
			if (oculusTracker->isGood()) {
				// update cvars TODO
#ifdef HAVE_OCULUS_API
				const OVR::HMDInfo& info = oculusTracker->getInfo();
				vr_ipd = info.InterpupillaryDistance;
#endif
			}
		}
		if (oculusTracker->isGood()) {
			oculusTracker->update(); // get new orientation from headset.

			// Yaw
			result.yaw = oculusTracker->yaw;
			// Pitch
			double pitch0 = oculusTracker->pitch;			
			// Correct pitch for doom pixel aspect ratio
			const double aspect = 1.20;
			result.pitch = atan( tan(pitch0) / aspect );

			// Roll can be local, because it doesn't affect gameplay.
			double rollAspect = 1.0 + (aspect - 1.0) * cos(result.pitch); // correct for pixel aspect
			double roll0 = -oculusTracker->roll;
			result.roll = atan2(rollAspect * sin(roll0), cos(roll0));
		}
	}

	return result;
}

void Stereo3D::setViewDirection(FGLRenderer& renderer) {
	// Set HMD angle parameters for NEXT frame
	static float previousYaw = 0;
	if (mode == OCULUS_RIFT) {
		PitchRollYaw prw = getHeadOrientation(renderer);
		if (oculusTracker->isGood()) {
			oculusTracker->update(); // get new orientation from headset.
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

