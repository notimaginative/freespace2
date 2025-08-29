uniform mat4 vOrtho;
attribute vec4 vPosition;
attribute vec4 vColor;
varying vec4 colorVar;

void main()
{
	gl_Position = vOrtho * vPosition;
	gl_PointSize = 1.5;
	colorVar = vColor;
}
