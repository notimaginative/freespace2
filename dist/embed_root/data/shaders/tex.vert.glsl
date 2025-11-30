uniform mat4 vOrtho;
attribute vec4 vPosition;
attribute vec4 vColor;
attribute vec2 vTexCoord;
varying vec4 colorVar;
varying vec2 texCoordVar;

void main()
{
	gl_Position = vOrtho * vPosition;
	gl_PointSize = 1.0;
	colorVar = vColor;
	texCoordVar = vTexCoord;
}
