uniform mat4 vOrtho;
attribute vec4 vPosition;
attribute vec4 vColor;
attribute vec4 vSecColor;
attribute float vPointSize;
varying vec4 colorVar;
varying vec4 secColorVar;

void main()
{
	gl_Position = vOrtho * vPosition;
	gl_PointSize = vPointSize;
	colorVar = vColor;
	secColorVar = vSecColor;
}
