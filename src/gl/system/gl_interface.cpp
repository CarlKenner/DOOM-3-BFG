/*
** r_opengl.cpp
**
** OpenGL system interface
**
**---------------------------------------------------------------------------
** Copyright 2005 Tim Stump
** Copyright 2005-2013 Christoph Oelckers
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
** 4. Full disclosure of the entire project's source code, except for third
**    party libraries is mandatory. (NOTE: This clause is non-negotiable!)
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
#include "gl/system/gl_system.h"
#include "tarray.h"
#include "doomtype.h"
#include "m_argv.h"
#include "zstring.h"
#include "version.h"
#include "i_system.h"
#include "v_text.h"
#include "gl/system/gl_interface.h"
#include "gl/system/gl_cvars.h"

static TArray<FString>  m_Extensions;

RenderContext gl;

int occlusion_type=0;

//==========================================================================
//
// 
//
//==========================================================================

static void CollectExtensions()
{
	const char *extension;

	int max = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &max);

	for(int i = 0; i < max; i++)
	{
		extension = (const char*)glGetStringi(GL_EXTENSIONS, i);
		m_Extensions.Push(FString(extension));
	}
}

//==========================================================================
//
// 
//
//==========================================================================

static bool CheckExtension(const char *ext)
{
	for (unsigned int i = 0; i < m_Extensions.Size(); ++i)
	{
		if (m_Extensions[i].CompareNoCase(ext) == 0) return true;
	}

	return false;
}



//==========================================================================
//
// 
//
//==========================================================================

static void InitContext()
{
	gl.flags=0;
}

//==========================================================================
//
// 
//
//==========================================================================

void gl_LoadExtensions()
{
	InitContext();
	CollectExtensions();

	const char *version = Args->CheckValue("-glversion");
	if (version == NULL) version = (const char*)glGetString(GL_VERSION);
	else Printf("Emulating OpenGL v %s\n", version);

	// Don't even start if it's lower than 3.0
	if (strcmp(version, "3.3") < 0)
	{
		I_FatalError("Unsupported OpenGL version.\nAt least OpenGL 3.3 is required to run " GAMENAME ".\n");
	}

	// add 0.01 to account for roundoff errors making the number a tad smaller than the actual version
	gl.version = strtod(version, NULL) + 0.01f;
	gl.glslversion = strtod((char*)glGetString(GL_SHADING_LANGUAGE_VERSION), NULL) + 0.01f;

	gl.vendorstring = (char*)glGetString(GL_VENDOR);

	if (CheckExtension("GL_ARB_texture_compression")) gl.flags|=RFL_TEXTURE_COMPRESSION;
	if (CheckExtension("GL_EXT_texture_compression_s3tc")) gl.flags|=RFL_TEXTURE_COMPRESSION_S3TC;
	if (!Args->CheckParm("-gl3"))
	{
		// don't use GL 4.x features when running in GL 3 emulation mode.
		if (CheckExtension("GL_ARB_shader_storage_buffer_object")) gl.flags |= RFL_SHADER_STORAGE_BUFFER;
		if (CheckExtension("GL_ARB_buffer_storage")) gl.flags |= RFL_BUFFER_STORAGE;
	}
	
	glGetIntegerv(GL_MAX_TEXTURE_SIZE,&gl.max_texturesize);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	
	if (gl.flags & RFL_GL_20)
	{
		// Rules:
		// SM4 will always use shaders. No option to switch them off is needed here.
		// SM3 has shaders optional but they are off by default (they will have a performance impact
		// SM2 only uses shaders for colormaps on camera textures and has no option to use them in general.
		//     On SM2 cards the shaders will be too slow and show visual bugs (at least on GF 6800.)
		if (strcmp((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION), "1.3") >= 0) gl.shadermodel = 4;
		else if (CheckExtension("GL_NV_GPU_shader4")) gl.shadermodel = 4;	// for pre-3.0 drivers that support GF8xxx.
		else if (CheckExtension("GL_EXT_GPU_shader4")) gl.shadermodel = 4;	// for pre-3.0 drivers that support GF8xxx.
		else if (CheckExtension("GL_NV_vertex_program3")) gl.shadermodel = 3;
		else if (!strstr(gl.vendorstring, "NVIDIA")) gl.shadermodel = 3;
		else gl.shadermodel = 2;	// Only for older NVidia cards which had notoriously bad shader support.

		// Command line overrides for testing and problem cases.
		if (Args->CheckParm("-sm2") && gl.shadermodel > 2) gl.shadermodel = 2;
		else if (Args->CheckParm("-sm3") && gl.shadermodel > 3) gl.shadermodel = 3;
	}

	if (CheckExtension("GL_ARB_map_buffer_range")) 
	{
		gl.flags|=RFL_MAP_BUFFER_RANGE;
	}

	if (gl.flags & RFL_GL_30 || CheckExtension("GL_EXT_framebuffer_object"))
	{
		gl.flags|=RFL_FRAMEBUFFER;
	}

}

//==========================================================================
//
// 
//
//==========================================================================

void gl_PrintStartupLog()
{
	Printf ("GL_VENDOR: %s\n", glGetString(GL_VENDOR));
	Printf ("GL_RENDERER: %s\n", glGetString(GL_RENDERER));
	Printf ("GL_VERSION: %s\n", glGetString(GL_VERSION));
	Printf ("GL_SHADING_LANGUAGE_VERSION: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	Printf ("GL_EXTENSIONS:");
	for (unsigned i = 0; i < m_Extensions.Size(); i++)
	{
		Printf(" %s", m_Extensions[i].GetChars());
	}
	int v = 0;

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &v);
	Printf("\nMax. texture size: %d\n", v);
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &v);
	Printf ("Max. texture units: %d\n", v);
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &v);
	Printf ("Max. fragment uniforms: %d\n", v);
	gl.maxuniforms = v;
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &v);
	Printf ("Max. vertex uniforms: %d\n", v);
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &v);
	Printf ("Max. uniform block size: %d\n", v);
	gl.maxuniformblock = v;
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &v);
	Printf ("Uniform block alignment: %d\n", v);
	gl.uniformblockalignment = v;

	glGetIntegerv(GL_MAX_VARYING_FLOATS, &v);
	Printf ("Max. varying: %d\n", v);
	glGetIntegerv(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, &v);
	Printf("Max. combined shader storage blocks: %d\n", v);
	glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &v);
	Printf("Max. vertex shader storage blocks: %d\n", v);


}

