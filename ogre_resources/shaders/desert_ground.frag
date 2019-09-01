#version 330

uniform vec3 sceneColor;

in vec3 color;
in vec2 tex_uv;
in vec2 bump_uv;

in vec3 bumpNormal;
in vec3 bumpLightSource;

uniform sampler2D texture;
uniform sampler2D bumpmap;

void main() {
	//texture2D(texture, tex_uv)
	//gl_FragColor = vec4(1, 1, 0, 1);// * texture2D(texture, tex_uv);

	vec3 normLightSource = normalize(bumpLightSource);
	float vertNormalProjection = dot(bumpNormal, normLightSource);
	float bumpProjection = pow(max(0, dot(vec3(texture2D(bumpmap, bump_uv)), normLightSource)), 1.25);
	float i = max(0.1, vertNormalProjection * bumpProjection);
	gl_FragColor = max(vec4(sceneColor, 1), pow(i, 4) * vec4(color, 1)) * texture2D(texture, tex_uv);
}
