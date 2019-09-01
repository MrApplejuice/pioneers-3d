#version 330

uniform vec3 sceneColor;

in vec3 color;
in vec2 tex_uv;
in vec2 bump_uv;

in float intensity;
in vec3 bumpLightSource;

uniform sampler2D texture;
uniform sampler2D bumpmap;

void main() {
	//texture2D(texture, tex_uv)
	//gl_FragColor = vec4(1, 1, 0, 1);// * texture2D(texture, tex_uv);

	vec3 normLightSource = normalize(bumpLightSource);
	float bumpProjection = max(0, dot(normalize(vec3(texture2D(bumpmap, bump_uv)) * 2 - vec3(1)), normLightSource));
	float i = max(0.1, intensity * pow(bumpProjection, 2));
	gl_FragColor = max(vec4(sceneColor, 1), i * vec4(color, 1) * 2) * texture2D(texture, tex_uv);
	//gl_FragColor = max(vec4(sceneColor, 1), pow(i, 4) * vec4(color, 1));
	//gl_FragColor = vec4(normLightSource, 1);//vec4(0.1 + 0.8 * bumpProjection * vec3(1, 1, 1), 1);
	//gl_FragColor.b = 0;
	//gl_FragColor.g = 0;
}
