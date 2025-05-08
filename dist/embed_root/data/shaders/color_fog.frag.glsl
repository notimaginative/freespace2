precision mediump float;
varying vec4 colorVar;
varying vec4 secColorVar;

void main()
{
	gl_FragColor = vec4(mix(secColorVar.rgb, colorVar.rgb, 1.0 - secColorVar.a), colorVar.a);
}