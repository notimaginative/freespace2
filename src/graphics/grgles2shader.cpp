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


static GLuint tex_prog = 0;
static GLuint fog_tex_prog = 0;
static GLuint nondark_prog = 0;
static GLuint fog_nondark_prog = 0;
static GLuint aabitmap_prog = 0;
static GLuint color_prog = 0;
static GLuint fog_color_prog = 0;
static GLuint window_prog = 0;


static const char v_tex_src[] =
	"uniform mat4 vOrtho;\n"
	"attribute vec4 vPosition;\n"
	"attribute vec4 vColor;\n"
	"attribute vec2 vTexCoord;\n"
	"varying vec4 colorVar;\n"
	"varying vec2 texCoordVar;\n"
	"void main()\n"
	"{\n"
	"	gl_Position = vOrtho * vPosition;\n"
	"	colorVar = vColor;\n"
	"	texCoordVar = vTexCoord;\n"
	"}\n";

static const char v_fog_tex_src[] =
	"uniform mat4 vOrtho;\n"
	"attribute vec4 vPosition;\n"
	"attribute vec4 vColor;\n"
	"attribute vec4 vSecColor;\n"
	"attribute vec2 vTexCoord;\n"
	"varying vec4 colorVar;\n"
	"varying vec4 secColorVar;\n"
	"varying vec2 texCoordVar;\n"
	"void main()\n"
	"{\n"
	"	gl_Position = vOrtho * vPosition;\n"
	"	colorVar = vColor;\n"
	"	secColorVar = vSecColor;\n"
	"	texCoordVar = vTexCoord;\n"
	"}\n";

static const char v_color_src[] =
	"uniform mat4 vOrtho;\n"
	"attribute vec4 vPosition;\n"
	"attribute vec4 vColor;\n"
	"varying vec4 colorVar;\n"
	"void main()\n"
	"{\n"
	"	gl_Position = vOrtho * vPosition;\n"
	"	colorVar = vColor;\n"
	"}\n";

static const char v_fog_color_src[] =
	"uniform mat4 vOrtho;\n"
	"attribute vec4 vPosition;\n"
	"attribute vec4 vColor;\n"
	"attribute vec4 vSecColor;\n"
	"varying vec4 colorVar;\n"
	"varying vec4 secColorVar;\n"
	"void main()\n"
	"{\n"
	"	gl_Position = vOrtho * vPosition;\n"
	"	colorVar = vColor;\n"
	"	secColorVar = vSecColor;\n"
	"}\n";

static const char v_window_src[] =
	"uniform mat4 vOrtho;\n"
	"attribute vec4 vPosition;\n"
	"attribute vec2 vTexCoord;\n"
	"varying vec2 texCoordVar;\n"
	"void main()\n"
	"{\n"
	"	gl_Position = vOrtho * vPosition;\n"
	"	texCoordVar = vTexCoord;\n"
	"}\n";

static const char f_tex_src[] =
	"precision mediump float;\n"
	"uniform sampler2D texture;\n"
	"varying vec4 colorVar;\n"
	"varying vec2 texCoordVar;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = colorVar * texture2D(texture, texCoordVar);\n"
	"}\n";

static const char f_fog_tex_src[] =
	"precision mediump float;\n"
	"uniform sampler2D texture;\n"
	"varying vec4 colorVar;\n"
	"varying vec4 secColorVar;\n"
	"varying vec2 texCoordVar;\n"
	"void main()\n"
	"{\n"
	"	vec4 base_color = colorVar * texture2D(texture, texCoordVar);\n"
	"	gl_FragColor = vec4(mix(secColorVar.rgb, base_color.rgb, 1.0 - secColorVar.a), base_color.a);\n"
	"}\n";

static const char f_nondark_src[] =
	"precision mediump float;\n"
	"uniform sampler2D texture;\n"
	"varying vec4 colorVar;\n"
	"varying vec2 texCoordVar;\n"
	"void main()\n"
	"{\n"
	"	vec4 tex_color = texture2D(texture, texCoordVar);\n"
	"	vec4 base_color = colorVar * vec4(tex_color.rgb, 1.0);\n"
	"	base_color.rgb += tex_color.rgb * tex_color.a;\n"
	" 	gl_FragColor = base_color;\n"
	"}\n";

static const char f_fog_nondark_src[] =
	"precision mediump float;\n"
	"uniform sampler2D texture;\n"
	"varying vec4 colorVar;\n"
	"varying vec4 secColorVar;\n"
	"varying vec2 texCoordVar;\n"
	"void main()\n"
	"{\n"
	"	vec4 tex_color = texture2D(texture, texCoordVar);\n"
	"	vec4 base_color = colorVar * vec4(tex_color.rgb, 1.0);\n"
	"	base_color.rgb += tex_color.rgb * tex_color.a;\n"
	"	gl_FragColor = vec4(mix(secColorVar.rgb, base_color.rgb, 1.0 - secColorVar.a), base_color.a);\n"
	"}\n";

static const char f_aabitmap_src[] =
	"precision mediump float;\n"
	"uniform sampler2D texture;\n"
	"varying vec4 colorVar;\n"
	"varying vec2 texCoordVar;\n"
	"void main()\n"
	"{\n"
	"	float alpha1 = texture2D(texture, texCoordVar).a;\n"
	"	gl_FragColor = vec4(colorVar.rgb, mix(0.0, colorVar.a, alpha1));\n"
	"}\n";

static const char f_color_src[] =
	"precision mediump float;\n"
	"varying vec4 colorVar;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = colorVar;\n"
	"}\n";

static const char f_fog_color_src[] =
	"precision mediump float;\n"
	"varying vec4 colorVar;\n"
	"varying vec4 secColorVar;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = vec4(mix(secColorVar.rgb, colorVar.rgb, 1.0 - secColorVar.a), colorVar.a);\n"
	"}\n";

static const char f_window_src[] =
	"precision mediump float;\n"
	"uniform sampler2D texture;\n"
	"varying vec2 texCoordVar;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D(texture, texCoordVar);\n"
	"}\n";


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

static GLuint gles2_create_program(GLuint vert, GLuint frag)
{
	GLuint program;
	GLint linked;

	program = pglCreateProgram();

	if ( !program ) {
		return 0;
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

		return 0;
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
	GLuint v_tex = gles2_create_shader(v_tex_src, GL_VERTEX_SHADER);
	GLuint v_fog_tex = gles2_create_shader(v_fog_tex_src, GL_VERTEX_SHADER);
	GLuint v_color = gles2_create_shader(v_color_src, GL_VERTEX_SHADER);
	GLuint v_fog_color = gles2_create_shader(v_fog_color_src, GL_VERTEX_SHADER);
	GLuint v_window = gles2_create_shader(v_window_src, GL_VERTEX_SHADER);

	GLuint f_aabitmap = gles2_create_shader(f_aabitmap_src, GL_FRAGMENT_SHADER);
	GLuint f_tex = gles2_create_shader(f_tex_src, GL_FRAGMENT_SHADER);
	GLuint f_fog_tex = gles2_create_shader(f_fog_tex_src, GL_FRAGMENT_SHADER);
	GLuint f_nondark = gles2_create_shader(f_nondark_src, GL_FRAGMENT_SHADER);
	GLuint f_fog_nondark = gles2_create_shader(f_fog_nondark_src, GL_FRAGMENT_SHADER);
	GLuint f_color = gles2_create_shader(f_color_src, GL_FRAGMENT_SHADER);
	GLuint f_fog_color = gles2_create_shader(f_fog_color_src, GL_FRAGMENT_SHADER);
	GLuint f_window = gles2_create_shader(f_window_src, GL_FRAGMENT_SHADER);

	aabitmap_prog = gles2_create_program(v_tex, f_aabitmap);
	tex_prog = gles2_create_program(v_tex, f_tex);
	fog_tex_prog = gles2_create_program(v_fog_tex, f_fog_tex);
	nondark_prog = gles2_create_program(v_tex, f_nondark);
	fog_nondark_prog = gles2_create_program(v_fog_tex, f_fog_nondark);
	color_prog = gles2_create_program(v_color, f_color);
	fog_color_prog = gles2_create_program(v_fog_color, f_fog_color);
	window_prog = gles2_create_program(v_window, f_window);


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
