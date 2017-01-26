#ifndef GZDOOM_GL_HudTEXTURE_H_
#define GZDOOM_GL_HudTEXTURE_H_

// Framebuffer texture for intermediate rendering of Hud image
class HudTexture {
public:
	HudTexture(int width, int height, float screenScale);
	~HudTexture() {destroy();}
	void bindToFrameBuffer();
	void bindRenderTexture();
	bool checkScreenSize(int screenWidth, int screenHeight); // is the size still the same?
	int getWidth() {return w;}
	int getHeight() {return h;}
	bool isBound() const {return m_isBound;}
	void renderToScreen();
	int getTexId() const { return renderedTexture; }
	void unbind();
	void setScreenScale(float s) {screenSizeScale = s;}

	// static methods act upon single instance of offscreen render buffer
	static void bindGlobalOffscreenBuffer();
	static void unbindGlobalOffscreenBuffer();
	static void displayAndClearGlobalOffscreenBuffer();
	static void bindAndClearGlobalOffscreenBuffer();

	// temporarily public TODO
	static HudTexture* hudTexture;
	static HudTexture* crosshairTexture;

private:
	void init(int width, int height);
	void destroy(); // release all opengl resources

	float screenSizeScale;
	unsigned int w, h;
	unsigned int frameBuffer;
	unsigned int renderedTexture;
	bool m_isBound;
};

#endif // GZDOOM_GL_HudTEXTURE_H_
