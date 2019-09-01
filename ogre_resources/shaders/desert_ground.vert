#version 330

attribute vec4 vertex;
attribute vec4 normal;
attribute vec4 uv0;
attribute vec4 uv1;

uniform mat4 worldMatrix;
uniform mat4 invWorldMatrix;
uniform mat4 viewProjMatrix;

uniform vec3 diffuse;

uniform vec3 lightPos;

out vec3 color;
out vec2 tex_uv;
out vec2 bump_uv;

out vec3 bumpNormal;
out vec3 bumpLightSource;

void main() {
	gl_Position = worldMatrix * vertex;

	bumpNormal = normalize(mat3(worldMatrix) * vec3(normal));

	vec3 lposfid = lightPos - vec3(gl_Position);
	float intensity =
			max(0, dot(bumpNormal, normalize(lposfid)))
			* max(0, (3 - length(lposfid)) / 3);

	gl_Position = viewProjMatrix * gl_Position;

	bumpLightSource = vec3(invWorldMatrix * vec4(lightPos, 1) - vertex);

	tex_uv = vec2(uv0);
	bump_uv = vec2(uv1);
	color = diffuse;
}
