#ifndef GL_OFFSCREENBUFFERMANAGER_H_
#define GL_OFFSCREENBUFFERMANAGER_H_

/**
 * Stack allocated class to temporarily render to offscreen buffer, then render to screen.
 * Nature of screen rendering may depend on current stereo 3D mode.
 */
class OffscreenBufferManager {
public:
	OffscreenBufferManager();
	virtual ~OffscreenBufferManager();
	static void bind();
private:
};

#endif // GL_OFFSCREENBUFFERMANAGER_H_

