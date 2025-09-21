/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#define SDL_USE_BUILTIN_OPENGL_DEFINITIONS
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

	sdr = GLES2_ctx.glCreateShader(type);

	if ( !sdr ) {
		return 0;
	}

	GLES2_ctx.glShaderSource(sdr, 1, &src, NULL);

	GLES2_ctx.glCompileShader(sdr);

	GLES2_ctx.glGetShaderiv(sdr, GL_COMPILE_STATUS, &compiled);

	if ( !compiled ) {
		GLint len = 0;

		GLES2_ctx.glGetShaderiv(sdr, GL_INFO_LOG_LENGTH, &len);

		if (len > 1) {
			char *log = (char *) malloc(sizeof(char) * len);

			GLES2_ctx.glGetShaderInfoLog(sdr, len, NULL, log);
			nprintf(("OpenGL", "Error compiling shader:\n%s\n", log));

			free(log);
		}

		GLES2_ctx.glDeleteShader(sdr);

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

	program = GLES2_ctx.glCreateProgram();

	if ( !program ) {
		throw "Shader program creation failed!";
	}

	GLES2_ctx.glAttachShader(program, vert);
	GLES2_ctx.glAttachShader(program, frag);

	GLES2_ctx.glBindAttribLocation(program, SDRI_POSITION, "vPosition");
	GLES2_ctx.glBindAttribLocation(program, SDRI_COLOR, "vColor");
	GLES2_ctx.glBindAttribLocation(program, SDRI_SEC_COLOR, "vSecColor");
	GLES2_ctx.glBindAttribLocation(program, SDRI_TEXCOORD, "vTexCoord");

	GLES2_ctx.glLinkProgram(program);

	GLES2_ctx.glGetProgramiv(program, GL_LINK_STATUS, &linked);

	if ( !linked ) {
		GLint len = 0;

		GLES2_ctx.glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);

		if (len > 1) {
			char *log = (char *) malloc(sizeof(char) * len);

			GLES2_ctx.glGetProgramInfoLog(program, len, NULL, log);
			nprintf(("OpenGL", "Error linking program:\n%s\n", log));

			free(log);
		}

		GLES2_ctx.glDeleteProgram(program);

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
			GLES2_ctx.glUseProgram(tex_prog);
			break;

		case PROG_NONDARK:
			GLES2_ctx.glUseProgram(nondark_prog);
			break;

		case PROG_AABITMAP:
			GLES2_ctx.glUseProgram(aabitmap_prog);
			break;

		case PROG_COLOR:
			GLES2_ctx.glUseProgram(color_prog);
			break;

		case PROG_WINDOW:
			GLES2_ctx.glUseProgram(window_prog);
			break;

		case PROG_TEX_FOG:
			GLES2_ctx.glUseProgram(fog_tex_prog);
			break;

		case PROG_NONDARK_FOG:
			GLES2_ctx.glUseProgram(fog_nondark_prog);
			break;

		case PROG_COLOR_FOG:
			GLES2_ctx.glUseProgram(fog_color_prog);
			break;

		default:
			Int3();
			break;
	}

	current = prog;
}

// update window ortho coords
void gles2_shader_update(int width, int height)
{
	GLfloat ortho[16];

	SDL_zero(ortho);

	if ( !width ) width = GLES2_viewport_w;
	if ( !height ) height = GLES2_viewport_h;

	ortho[0] = 2.0f / width;
	ortho[5] = 2.0f / -height;
	ortho[10] = -2.0f / 1.0f;
	ortho[12] = -1.0f;
	ortho[13] = 1.0f;
	ortho[14] = -1.0f;
	ortho[15] = 1.0f;

	gles2_shader_use(PROG_WINDOW);
	GLint loc = GLES2_ctx.glGetUniformLocation(window_prog, "vOrtho");
	GLES2_ctx.glUniformMatrix4fv(loc, 1, GL_FALSE, ortho);
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
		(void)err;
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
	loc = GLES2_ctx.glGetUniformLocation(fog_color_prog, "vOrtho");
	GLES2_ctx.glUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	gles2_shader_use(PROG_COLOR);
	loc = GLES2_ctx.glGetUniformLocation(color_prog, "vOrtho");
	GLES2_ctx.glUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	gles2_shader_use(PROG_TEX_FOG);
	loc = GLES2_ctx.glGetUniformLocation(fog_tex_prog, "vOrtho");
	GLES2_ctx.glUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	gles2_shader_use(PROG_AABITMAP);
	loc = GLES2_ctx.glGetUniformLocation(aabitmap_prog, "vOrtho");
	GLES2_ctx.glUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	gles2_shader_use(PROG_TEX);
	loc = GLES2_ctx.glGetUniformLocation(tex_prog, "vOrtho");
	GLES2_ctx.glUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	gles2_shader_use(PROG_NONDARK);
	loc = GLES2_ctx.glGetUniformLocation(nondark_prog, "vOrtho");
	GLES2_ctx.glUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	gles2_shader_use(PROG_NONDARK_FOG);
	loc = GLES2_ctx.glGetUniformLocation(fog_nondark_prog, "vOrtho");
	GLES2_ctx.glUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	gles2_shader_update();

	return 1;
}

void gles2_shader_cleanup()
{
	if (tex_prog) {
		GLES2_ctx.glDeleteProgram(tex_prog);
		tex_prog = 0;
	}

	if (fog_tex_prog) {
		GLES2_ctx.glDeleteProgram(fog_tex_prog);
		fog_tex_prog = 0;
	}

	if (aabitmap_prog) {
		GLES2_ctx.glDeleteProgram(aabitmap_prog);
		aabitmap_prog = 0;
	}

	if (color_prog) {
		GLES2_ctx.glDeleteProgram(color_prog);
		color_prog = 0;
	}

	if (fog_color_prog) {
		GLES2_ctx.glDeleteProgram(fog_color_prog);
		fog_color_prog = 0;
	}

	if (window_prog) {
		GLES2_ctx.glDeleteProgram(window_prog);
		window_prog = 0;
	}

	if (nondark_prog) {
		GLES2_ctx.glDeleteProgram(nondark_prog);
		nondark_prog = 0;
	}

	if (fog_nondark_prog) {
		GLES2_ctx.glDeleteProgram(fog_nondark_prog);
		fog_nondark_prog = 0;
	}
}
