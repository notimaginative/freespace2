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


GLuint basicTexture = 0;


static const char vert_src[] =
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

static const char frag_src[] =
	"precision mediump float;\n"
	"uniform sampler2D texture;\n"
	"varying vec4 colorVar;\n"
	"varying vec2 texCoordVar;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = colorVar * texture2D(texture, texCoordVar);\n"
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
/*
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

	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &linked);

	if ( !linked ) {
		GLint len = 0;

		glGetProgramiv(basicTexture, GL_INFO_LOG_LENGTH, &len);

		if (len > 1) {
			char *log = (char *) malloc(sizeof(char) * len);

			glGetProgramInfoLog(basicTexture, len, NULL, log);
			nprintf(("OpenGL", "Error linking program:\n%s\n", log));

			free(log);
		}

		glDeleteProgram(basicTexture);

		return 0;
	}

	return program;
}
*/
int opengl2_shader_init()
{
	GLint linked;

	GLuint vert_shader = opengl2_create_shader(vert_src, GL_VERTEX_SHADER);
	GLuint frag_shader = opengl2_create_shader(frag_src, GL_FRAGMENT_SHADER);

	basicTexture = glCreateProgram();

	if ( !basicTexture ) {
		return 0;
	}

	glAttachShader(basicTexture, vert_shader);
	glAttachShader(basicTexture, frag_shader);

	glBindAttribLocation(basicTexture, 1, "vPosition");
	glBindAttribLocation(basicTexture, 2, "vColor");
	glBindAttribLocation(basicTexture, 3, "vTexCoord");

	glLinkProgram(basicTexture);

	glGetProgramiv(basicTexture, GL_LINK_STATUS, &linked);

	if ( !linked ) {
		GLint len = 0;

		glGetProgramiv(basicTexture, GL_INFO_LOG_LENGTH, &len);

		if (len > 1) {
			char *log = (char *) malloc(sizeof(char) * len);

			glGetProgramInfoLog(basicTexture, len, NULL, log);
			nprintf(("OpenGL", "Error linking program:\n%s\n", log));

			free(log);
		}

		glDeleteProgram(basicTexture);

		return 0;
	}

	glUseProgram(basicTexture);

	return 1;
}

void opengl2_shader_cleanup()
{
	if (basicTexture) {
		glDeleteProgram(basicTexture);
		basicTexture = 0;
	}
}
