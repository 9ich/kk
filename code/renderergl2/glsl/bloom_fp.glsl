// bloom colour stepping

uniform sampler2D u_TextureMap;
uniform vec4      u_Color;
varying vec2      var_TexCoords;

void main()
{
	float thresh = 1.0;
	vec4 clr = texture2D(u_TextureMap, var_TexCoords);
	float luma = (0.2126*clr.r) + (0.7152*clr.g) + (0.0722*clr.b);
	float x = step(1.0, luma);	// thresh = 1.0
	gl_FragColor = vec4(x*clr.rgb, 1.0);
}
