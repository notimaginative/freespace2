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


static GLuint tex_prog = 0;
static GLuint fog_tex_prog = 0;
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

static const char f_aabitmap_src[] =
	"precision mediump float;\n"
	"uniform sampler2D texture;\n"
	"varying vec4 colorVar;\n"
	"varying vec2 texCoordVar;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = colorVar * texture2D(texture, texCoordVar).aaaa;\n"
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
		case PROG_TEX:
			glUseProgram(tex_prog);
			break;

		case PROG_AABITMAP:
			glUseProgram(aabitmap_prog);
			break;

		case PROG_COLOR:
			glUseProgram(color_prog);
			break;

		case PROG_WINDOW:
			glUseProgram(window_prog);
			break;

		case PROG_TEX_FOG:
			glUseProgram(fog_tex_prog);
			break;

		case PROG_COLOR_FOG:
			glUseProgram(fog_color_prog);
			break;

		default:
			Int3();
			break;
	}

	current = prog;
}

// update window ortho coords
void opengl2_shader_update()
{
	GLfloat ortho[16];

	SDL_zero(ortho);

	ortho[0] = 2.0f / GL_viewport_w;
	ortho[5] = 2.0f / -GL_viewport_h;
	ortho[10] = -2.0f / 1.0f;
	ortho[12] = -1.0f;
	ortho[13] = 1.0f;
	ortho[14] = -1.0f;
	ortho[15] = 1.0f;

	opengl2_shader_use(PROG_WINDOW);
	GLint loc = glGetUniformLocation(window_prog, "vOrtho");
	glUniformMatrix4fv(loc, 1, GL_FALSE, ortho);
}

int opengl2_shader_init()
{
	GLuint v_tex = opengl2_create_shader(v_tex_src, GL_VERTEX_SHADER);
	GLuint v_fog_tex = opengl2_create_shader(v_fog_tex_src, GL_VERTEX_SHADER);
	GLuint v_color = opengl2_create_shader(v_color_src, GL_VERTEX_SHADER);
	GLuint v_fog_color = opengl2_create_shader(v_fog_color_src, GL_VERTEX_SHADER);
	GLuint v_window = opengl2_create_shader(v_window_src, GL_VERTEX_SHADER);

	GLuint f_aabitmap = opengl2_create_shader(f_aabitmap_src, GL_FRAGMENT_SHADER);
	GLuint f_tex = opengl2_create_shader(f_tex_src, GL_FRAGMENT_SHADER);
	GLuint f_fog_tex = opengl2_create_shader(f_fog_tex_src, GL_FRAGMENT_SHADER);
	GLuint f_color = opengl2_create_shader(f_color_src, GL_FRAGMENT_SHADER);
	GLuint f_fog_color = opengl2_create_shader(f_fog_color_src, GL_FRAGMENT_SHADER);
	GLuint f_window = opengl2_create_shader(f_window_src, GL_FRAGMENT_SHADER);

	aabitmap_prog = opengl2_create_program(v_tex, f_aabitmap);
	tex_prog = opengl2_create_program(v_tex, f_tex);
	fog_tex_prog = opengl2_create_program(v_fog_tex, f_fog_tex);
	color_prog = opengl2_create_program(v_color, f_color);
	fog_color_prog = opengl2_create_program(v_fog_color, f_fog_color);
	window_prog = opengl2_create_program(v_window, f_window);


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

	opengl2_shader_use(PROG_COLOR_FOG);
	loc = glGetUniformLocation(fog_color_prog, "vOrtho");
	glUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	opengl2_shader_use(PROG_COLOR);
	loc = glGetUniformLocation(color_prog, "vOrtho");
	glUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	opengl2_shader_use(PROG_TEX_FOG);
	loc = glGetUniformLocation(fog_tex_prog, "vOrtho");
	glUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	opengl2_shader_use(PROG_AABITMAP);
	loc = glGetUniformLocation(aabitmap_prog, "vOrtho");
	glUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	opengl2_shader_use(PROG_TEX);
	loc = glGetUniformLocation(tex_prog, "vOrtho");
	glUniformMatrix4fv(loc, 1, GL_FALSE, ortho);

	opengl2_shader_update();

	return 1;
}

void opengl2_shader_cleanup()
{
	if (tex_prog) {
		glDeleteProgram(tex_prog);
		tex_prog = 0;
	}

	if (fog_tex_prog) {
		glDeleteProgram(fog_tex_prog);
		fog_tex_prog = 0;
	}

	if (aabitmap_prog) {
		glDeleteProgram(aabitmap_prog);
		aabitmap_prog = 0;
	}

	if (color_prog) {
		glDeleteProgram(color_prog);
		color_prog = 0;
	}

	if (fog_color_prog) {
		glDeleteProgram(fog_color_prog);
		fog_color_prog = 0;
	}

	if (window_prog) {
		glDeleteProgram(window_prog);
		window_prog = 0;
	}
}
