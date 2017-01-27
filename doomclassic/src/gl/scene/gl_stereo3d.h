#ifndef GZDOOM_GL_STEREO3D_H_
#define GZDOOM_GL_STEREO3D_H_

#include "gl/renderer/gl_renderer.h"
// #include "gl/scene/gl_hudtexture.h"
#include "gl/scene/gl_rift_hmd.h"
// #include "gl/scene/gl_oculustracker.h"

struct PitchRollYaw {
	float pitch; // radians
	float roll; // radians
	float yaw; // radians
};
	 
/**
 * Stereo3D class controls OpenGL viewport and projection matrix,
 * to drive various stereoscopic modes.
 */
class Stereo3D {
public:
	// Which Sterescopic 3D modes are available?
	enum Mode {
		MONO = 0,
		GREEN_MAGENTA,
		RED_CYAN,
		SIDE_BY_SIDE,
		SIDE_BY_SIDE_SQUISHED,
		LEFT_EYE_VIEW,
		RIGHT_EYE_VIEW,
		QUAD_BUFFERED,
		OCULUS_RIFT,
		YELLOW_BLUE, // TODO
		ROW_INTERLEAVED, // TODO
		CHECKERBOARD // TODO
	};

	static void setBufferHud(bool doBuffer);

	Stereo3D();

	Mode getMode() {return mode;}

	bool hasHeadTracking() const;
	PitchRollYaw getHeadOrientation(FGLRenderer& renderer);

	// Render OpenGL scene for both eyes. Delegated from FLGRenderer drawing routines.
	void render(FGLRenderer& renderer, GL_IRECT * bounds, float fov, float ratio, float fovratio, bool toscreen, sector_t * viewsector, player_t * player);

	// Adjusts player orientation on following frame, if OCULUS_RIFT mode is active. Otherwise does nothing.
	void setViewDirection(FGLRenderer& renderer);

	// Calls screen->Update(), but only after flushing stereo 3d render buffers, if any.
	void updateScreen(); 
	int getScreenWidth();
	int getScreenHeight();

	void bindHudTexture(bool doUse);
	void resetPosition();

protected:
	// Change stereo mode. Users should adjust this with vr_mode CVAR
	void setMode(int m);

private:

	void setMonoView(FGLRenderer& renderer, float fov, float ratio, float fovratio, player_t * player);
	void setLeftEyeView(FGLRenderer& renderer, float fov, float ratio, float fovratio, player_t * player, bool frustumShift=true);
	void setRightEyeView(FGLRenderer& renderer, float fov, float ratio, float fovratio, player_t * player, bool frustumShift=true);
	void setViewportFull(FGLRenderer& renderer, GL_IRECT * bounds);
	void setViewportLeft(FGLRenderer& renderer, GL_IRECT * bounds);
	void setViewportRight(FGLRenderer& renderer, GL_IRECT * bounds);
	void blitHudTextureToScreen(float yScale = 1.0);

	Mode mode; // Current 3D method
	// OculusTexture* oculusTexture; // Offscreen render buffer for pre-warped Oculus stereo view.
	// OculusTracker* oculusTracker; // Reads head orientation from Oculus Rift
	// HudTexture* hudTexture; // Offscreen render buffer for non-3D content for one eye.

};

extern Stereo3D Stereo3DMode;

#endif // GZDOOM_GL_STEREO3D_H_

