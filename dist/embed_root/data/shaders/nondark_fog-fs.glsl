precision mediump float;
uniform sampler2D texture;
varying vec4 colorVar;
varying vec4 secColorVar;
varying vec2 texCoordVar;

void main()
{
	vec4 tex_color = texture2D(texture, texCoordVar);
	vec4 base_color = colorVar * vec4(tex_color.rgb, 1.0);
	base_color.rgb += tex_color.rgb * tex_color.a;
	gl_FragColor = vec4(mix(secColorVar.rgb, base_color.rgb, 1.0 - secColorVar.a), base_color.a);
}