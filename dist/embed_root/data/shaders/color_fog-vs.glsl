uniform mat4 vOrtho;
attribute vec4 vPosition;
attribute vec4 vColor;
attribute vec4 vSecColor;
varying vec4 colorVar;
varying vec4 secColorVar;

void main()
{
	gl_Position = vOrtho * vPosition;
	colorVar = vColor;
	secColorVar = vSecColor;
}
