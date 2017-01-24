#ifndef GZDOOM_GL_LOCAL_HUD_RENDERER_H_
#define GZDOOM_GL_LOCAL_HUD_RENDERER_H_

/**
 * Stack allocated class for temporarily rendering to Oculus Rift HUD texture.
 */
class LocalHudRenderer {
public:
	static void bind();
	static void unbind();

	LocalHudRenderer();
	~LocalHudRenderer();

private:
};

#endif // GZDOOM_GL_LOCAL_HUD_RENDERER_H_
