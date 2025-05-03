uniform mat4 vOrtho;
attribute vec4 vPosition;
attribute vec2 vTexCoord;
varying vec2 texCoordVar;

void main()
{
	gl_Position = vOrtho * vPosition;
	texCoordVar = vTexCoord;
}