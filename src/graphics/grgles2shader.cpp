/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include <SDL3/SDL_opengles2.h>

#include "pstypes.h"
#include "grinternal.h"
#include "grgles2.h"
#include "grgles2internal.h"
#include "cfile.h"


static GLuint tex_prog = 0;
static GLuint fog_tex_prog = 0;
static GLuint nondark_prog = 0;
static GLuint fog_nondark_prog = 0;
static GLuint aabitmap_prog = 0;
static GLuint color_prog = 0;
static GLuint fog_color_prog = 0;
static GLuint window_prog = 0;


static GLuint gles2_create_shader(const char *src, GLenum type)
{
	GLuint sdr;
	GLint compiled;

	sdr = pglCreateShader(type);

	if ( !sdr ) {
		return 0;
	}

	pglShaderSource(sdr, 1, &src, NULL);

	pglCompileShader(sdr);

	pglGetShaderiv(sdr, GL_COMPILE_STATUS, &compiled);

	if ( !compiled ) {
		GLint len = 0;

		pglGetShaderiv(sdr, GL_INFO_LOG_LENGTH, &len);

		if (len > 1) {
			char *log = (char *) malloc(sizeof(char) * len);

			pglGetShaderInfoLog(sdr, len, NULL, log);
			nprintf(("OpenGL", "Error compiling shader:\n%s\n", log));

			free(log);
		}

		pglDeleteShader(sdr);

		return 0;
	}

	return sdr;
}

static GLuint gles2_load_shader(const char *name)
{
	GLenum shader_type = 0;

	if (SDL_strstr(name, ".vert")) {
		shader_type = GL_VERTEX_SHADER;
	} else if (SDL_strstr(name, ".frag")) {
		shader_type = GL_FRAGMENT_SHADER;
	}

	if (shader_type == 0) {
		throw "Unknown shader type!";
	}

	auto shader_source = reinterpret_cast<char *>(cf_load_file(name, "rt", CF_TYPE_SHADERS));

	if ( !shader_source ) {
		throw "Unable to read shader file!";
	}

	auto rval = gles2_create_shader(shader_source, shader_type);

	free(shader_source);

	if ( !rval ) {
		throw "Unable to create shader!";
	}

	return rval;
}

static GLuint gles2_create_program(GLuint vert, GLuint frag)
{
	GLuint program;
	GLint linked;

	program = pglCreateProgram();

	if ( !program ) {
		throw "Shader program creation failed!";
	}

	pglAttachShader(program, vert);
	pglAttachShader(program, frag);

	pglBindAttribLocation(program, SDRI_POSITION, "vPosition");
	pglBindAttribLocation(program, SDRI_COLOR, "vColor");
	pglBindAttribLocation(program, SDRI_SEC_COLOR, "vSecColor");
	pglBindAttribLocation(program, SDRI_TEXCOORD, "vTexCoord");

	pglLinkProgram(program);

	pglGetProgramiv(program, GL_LINK_STATUS, &linked);

	if ( !linked ) {
		GLint len = 0;

		pglGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);

		if (len > 1) {
			char *log = (char *) malloc(sizeof(char) * len);

			pglGetProgramInfoLog(program, len, NULL, log);
			nprintf(("OpenGL", "Error linking program:\n%s\n", log));

			free(log);
		}

		pglDeleteProgram(program);

		throw "Shader program linking failed!";
	}

	return program;
}

void gles2_shader_use(sdr_prog_t prog)
{
	static sdr_prog_t current = PROG_INVALID;

	if (prog == current) {
		return;
	}

	switch (prog) {
		case PROG_TEX:
			pglUseProgram(tex_prog);
			break;

		case PROG_NONDARK:
			pglUseProgram(nondark_prog);
			break;

		case PROG_AABITMAP:
			pglUseProgram(aabitmap_prog);
			break;

		case PROG_COLOR:
			pglUseProgram(color_prog);
			break;

		case PROG_WINDOW:
			pglUseProgram(window_prog);
			break;

		case PROG_TEX_FOG:
			pglUseProgram(fog_tex_prog);
			break;

		case PROG_NONDARK_FOG:
			pglUseProgram(fog_nondark_prog);
			break;

		case PROG_COLOR_FOG:
			pglUseProgram(fog_color_prog);
			break;

		default:
			Int3();
			break;
	}

	current = prog;
}

// update window ortho coords
void gles2_shader_update()
{
	GLfloat ortho[16];

	SDL_zero(ortho);

	ortho[0] = 2.0f / GLES2_viewport_w;
	ortho[5] = 2.0f / -GLES2_viewport_h;
	ortho[10] = -2.0f / 1.0f;
	ortho[12] = -1.0f;
	ortho[13] = 1.0f;
	ortho[14] = -1.0f;
	ortho[15] = 1.0f;

	gles2_shader_use(PROG_WINDOW);
	GLint loc = pglGetUniformLocation(window_prog, "vOrtho");
	pglUniformMatrix4fv(loc, 1, GL_FALSE, ortho);
}

int gles2_shader_init()
{
	try {
		GLuint v_tex = gles2_load_shader("tex.vert.glsl");
		GLuint v_fog_tex = gles2_load_shader("tex_fog.vert.glsl");
		GLuint v_color = gles2_load_shader("color.vert.glsl");
		GLuint v_fog_color = gles2_load_shader("color_fog.vert.glsl");
		GLuint v_window = gles2_load_shader("window.vert.glsl");

		GLuint f_aabitmap = gles2_load_shader("aabitmap.frag.glsl");
		GLuint f_tex = gles2_load_shader("tex.frag.glsl");
		GLuint f_fog_tex = gles2_load_shader("tex_fog.frag.glsl");
		GLuint f_nondark = gles2_load_shader("nondark.frag.glsl");
		GLuint f_fog_nondark = gles2_load_shader("nondark_fog.frag.glsl");
		GLuint f_color = gles2_load_shader("color.frag.glsl");
		GLuint f_fog_color = gles2_load_shader("color_fog.frag.glsl");
		GLuint f_window = gles2_load_shader("window.frag.glsl");

		aabitmap_prog = gles2_create_program(v_tex, f_aabitmap);
		tex_prog = gles2_create_program(v_tex, f_tex);
		fog_tex_prog = gles2_create_program(v_fog_tex, f_fog_tex);
		nondark_prog = gles2_create_program(v_tex, f_nondark);
		fog_nondark_prog = gles2_create_program(v_fog_tex, f_fog_nondark);
		color_prog = gles2_create_program(v_color, f_color);
		fog_color_prog = gles2_create_program(v_fog_color, f_fog_color);
		window_prog = gles2_create_program(v_window, f_window);
	} catch (const char *err) {
		nprintf(("OpenGL", "Shader ERROR: %s\n", err));
		gles2_shader_cleanup();
		return 0;
	}



	// set up orthographic projection var
	// (this should never have to change while game is running)
	GLfloat ortho[16];

	SDL_zero(ortho);

	ortho[0] = 2.0f / gr_screen.max_w;
	ortho[5] = 2.0f / -gr_screen.max_h;
	ortho[10] = -2.0f / 1.0f;
	ortho[12] = -1.0f;
	ortho[13] = 1.0f;
	ortho[14] = -1.0f;
	ortho[15] = 1.0f;

	GLint loc;

	gles2_shader_use(PROG_COLOR_FOG);
	loc = pglGetUniformLocation(fog_color_prog, "vOrtho");
	pglUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	gles2_shader_use(PROG_COLOR);
	loc = pglGetUniformLocation(color_prog, "vOrtho");
	pglUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	gles2_shader_use(PROG_TEX_FOG);
	loc = pglGetUniformLocation(fog_tex_prog, "vOrtho");
	pglUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	gles2_shader_use(PROG_AABITMAP);
	loc = pglGetUniformLocation(aabitmap_prog, "vOrtho");
	pglUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	gles2_shader_use(PROG_TEX);
	loc = pglGetUniformLocation(tex_prog, "vOrtho");
	pglUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	gles2_shader_use(PROG_NONDARK);
	loc = pglGetUniformLocation(nondark_prog, "vOrtho");
	pglUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	gles2_shader_use(PROG_NONDARK_FOG);
	loc = pglGetUniformLocation(fog_nondark_prog, "vOrtho");
	pglUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	gles2_shader_update();

	return 1;
}

void gles2_shader_cleanup()
{
	if (tex_prog) {
		pglDeleteProgram(tex_prog);
		tex_prog = 0;
	}

	if (fog_tex_prog) {
		pglDeleteProgram(fog_tex_prog);
		fog_tex_prog = 0;
	}

	if (aabitmap_prog) {
		pglDeleteProgram(aabitmap_prog);
		aabitmap_prog = 0;
	}

	if (color_prog) {
		pglDeleteProgram(color_prog);
		color_prog = 0;
	}

	if (fog_color_prog) {
		pglDeleteProgram(fog_color_prog);
		fog_color_prog = 0;
	}

	if (window_prog) {
		pglDeleteProgram(window_prog);
		window_prog = 0;
	}

	if (nondark_prog) {
		pglDeleteProgram(nondark_prog);
		nondark_prog = 0;
	}

	if (fog_nondark_prog) {
		pglDeleteProgram(fog_nondark_prog);
		fog_nondark_prog = 0;
	}
}
