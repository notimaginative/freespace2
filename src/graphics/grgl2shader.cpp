/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include "SDL_opengles2.h"

#include "pstypes.h"
#include "gropengl.h"
#include "gropenglinternal.h"
#include "grgl2.h"


static GLuint tmapper_prog = 0;
static GLuint aabitmap_prog = 0;
static GLuint lines_prog = 0;

static const char v_generic_tex_src[] =
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

static const char v_lines_src[] =
	"uniform mat4 vOrtho;\n"
	"attribute vec4 vPosition;\n"
	"attribute vec4 vColor;\n"
	"varying vec4 colorVar;\n"
	"void main()\n"
	"{\n"
	"	gl_Position = vOrtho * vPosition;\n"
	"	colorVar = vColor;\n"
	"}\n";

static const char f_tmapper_src[] =
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

static const char f_aabitmap_src[] =
	"precision mediump float;\n"
	"uniform sampler2D texture;\n"
	"varying vec4 colorVar;\n"
	"varying vec2 texCoordVar;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = colorVar * texture2D(texture, texCoordVar).aaaa;\n"
	"}\n";

static const char f_lines_src[] =
	"precision mediump float;\n"
	"varying vec4 colorVar;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = colorVar;\n"
	"}\n";



static GLuint opengl2_create_shader(const char *src, GLenum type)
{
	GLuint shader;
	GLint compiled;

	shader = glCreateShader(type);

	if ( !shader ) {
		return 0;
	}

	glShaderSource(shader, 1, &src, NULL);

	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if ( !compiled ) {
		GLint len = 0;

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

		if (len > 1) {
			char *log = (char *) malloc(sizeof(char) * len);

			glGetShaderInfoLog(shader, len, NULL, log);
			nprintf(("OpenGL", "Error compiling shader:\n%s\n", log));

			free(log);
		}

		glDeleteShader(shader);

		return 0;
	}

	return shader;
}

static GLuint opengl2_create_program(GLuint vert, GLuint frag)
{
	GLuint program;
	GLint linked;

	program = glCreateProgram();

	if ( !program ) {
		return 0;
	}

	glAttachShader(program, vert);
	glAttachShader(program, frag);

	glBindAttribLocation(program, SDRI_POSITION, "vPosition");
	glBindAttribLocation(program, SDRI_COLOR, "vColor");
	glBindAttribLocation(program, SDRI_SEC_COLOR, "vSecColor");
	glBindAttribLocation(program, SDRI_TEXCOORD, "vTexCoord");

	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &linked);

	if ( !linked ) {
		GLint len = 0;

		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);

		if (len > 1) {
			char *log = (char *) malloc(sizeof(char) * len);

			glGetProgramInfoLog(program, len, NULL, log);
			nprintf(("OpenGL", "Error linking program:\n%s\n", log));

			free(log);
		}

		glDeleteProgram(program);

		return 0;
	}

	return program;
}

void opengl2_shader_use(sdr_prog_t prog)
{
	static sdr_prog_t current = PROG_INVALID;

	if (prog == current) {
		return;
	}

	switch (prog) {
		case PROG_TMAPPER:
			glUseProgram(tmapper_prog);
			break;

		case PROG_AABITMAP:
			glUseProgram(aabitmap_prog);
			break;

		case PROG_LINES:
			glUseProgram(lines_prog);
			break;

		default:
			Int3();
			break;
	}

	current = prog;
}

int opengl2_shader_init()
{
	GLuint v_generic_tex = opengl2_create_shader(v_generic_tex_src, GL_VERTEX_SHADER);
	GLuint v_lines = opengl2_create_shader(v_lines_src, GL_VERTEX_SHADER);

	GLuint f_tmapper = opengl2_create_shader(f_tmapper_src, GL_FRAGMENT_SHADER);
	GLuint f_aabitmap = opengl2_create_shader(f_aabitmap_src, GL_FRAGMENT_SHADER);
	GLuint f_lines = opengl2_create_shader(f_lines_src, GL_FRAGMENT_SHADER);

	tmapper_prog = opengl2_create_program(v_generic_tex, f_tmapper);
	aabitmap_prog = opengl2_create_program(v_generic_tex, f_aabitmap);
	lines_prog = opengl2_create_program(v_lines, f_lines);


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

	opengl2_shader_use(PROG_LINES);
	loc = glGetUniformLocation(lines_prog, "vOrtho");
	glUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	opengl2_shader_use(PROG_AABITMAP);
	loc = glGetUniformLocation(aabitmap_prog, "vOrtho");
	glUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	opengl2_shader_use(PROG_TMAPPER);
	loc = glGetUniformLocation(tmapper_prog, "vOrtho");
	glUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	return 1;
}

void opengl2_shader_cleanup()
{
	if (tmapper_prog) {
		glDeleteProgram(tmapper_prog);
		tmapper_prog = 0;
	}

	if (aabitmap_prog) {
		glDeleteProgram(aabitmap_prog);
		aabitmap_prog = 0;
	}

	if (lines_prog) {
		glDeleteProgram(lines_prog);
		lines_prog = 0;
	}
}
