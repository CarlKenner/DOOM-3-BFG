
#ifndef GZDOOM_GL_COLORMASK_H
#define GZDOOM_GL_COLORMASK_H

#include <GL/glew.h>

/**
 *  RAII wrapper for glColorMask.
 *
 *  If you use stack allocated instances of this class,
 *  in place of all glColorMask calls, 
 *  the previous glColorMask is guaranteed to be restored when
 *  the local scope is exited.
 *  
 *  This can be helpful when glColorMask is being used in different
 *  ways by different parts of the program. In particular, glColorMask
 *  is used to temporarily disable color rendering, and to colorize
 *  stereoscopic 3D eye frames for anaglyph presentation.
 */
class LocalScopeGLColorMask {
public:
	LocalScopeGLColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) 
		: isPushed(false)
	{
		setColorMask(r, g, b, a);
	}

	~LocalScopeGLColorMask() {
		revert(); // Pop when exiting scope
	}

	void revert() {
		if (isPushed) {
			glPopAttrib();
			isPushed = false;
		}
	}

	void setColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) {
		if (! isPushed) {
			glPushAttrib(GL_COLOR_BUFFER_BIT);
			isPushed = true;
		}
		glColorMask(r,g,b,a);
	}

private:
	bool isPushed;
};

#endif // GZDOOM_GL_COLORMASK_H
