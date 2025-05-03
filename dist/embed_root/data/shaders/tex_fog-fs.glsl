precision mediump float;
uniform sampler2D texture;
varying vec4 colorVar;
varying vec4 secColorVar;
varying vec2 texCoordVar;

void main()
{
	vec4 base_color = colorVar * texture2D(texture, texCoordVar);
	gl_FragColor = vec4(mix(secColorVar.rgb, base_color.rgb, 1.0 - secColorVar.a), base_color.a);
}