precision mediump float;
uniform sampler2D texture;
varying vec4 colorVar;
varying vec2 texCoordVar;

void main()
{
	gl_FragColor = colorVar * texture2D(texture, texCoordVar);
}