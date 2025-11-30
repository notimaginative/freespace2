uniform mat4 vOrtho;
attribute vec4 vPosition;
attribute vec4 vColor;
attribute vec4 vSecColor;
attribute vec2 vTexCoord;
varying vec4 colorVar;
varying vec4 secColorVar;
varying vec2 texCoordVar;

void main()
{
	gl_Position = vOrtho * vPosition;
	gl_PointSize = 1.0;
	colorVar = vColor;
	secColorVar = vSecColor;
	texCoordVar = vTexCoord;
}
