precision mediump float;
uniform sampler2D texture;
varying vec4 colorVar;
varying vec2 texCoordVar;

void main()
{
	float alpha1 = texture2D(texture, texCoordVar).a;
	gl_FragColor = vec4(colorVar.rgb, mix(0.0, colorVar.a, alpha1));
}