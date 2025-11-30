uniform mat4 vOrtho;
attribute vec4 vPosition;
attribute vec4 vColor;
attribute float vPointSize;
varying vec4 colorVar;

void main()
{
	gl_Position = vOrtho * vPosition;
	gl_PointSize = vPointSize;
	colorVar = vColor;
}
