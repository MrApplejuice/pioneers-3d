#version 330

attribute vec4 vertex;
attribute vec4 normal;
attribute vec4 uv0;

uniform mat4 worldMatrix;
uniform mat4 viewProjMatrix;

uniform vec3 sceneColor;
uniform vec3 diffuse;

uniform vec3 lightPos;

out vec3 color;
out vec2 tex_uv;

void main() {
	gl_Position = worldMatrix * vertex;
	vec3 worldNormal = normalize(mat3(worldMatrix) * vec3(normal));
	vec3 lposfid = lightPos - vec3(gl_Position);;
	float intensity =
			max(0, dot(normalize(worldNormal), normalize(lposfid)))
			* max(0, (3 - length(lposfid)) / 3);
	gl_Position = viewProjMatrix * gl_Position;

	tex_uv = vec2(uv0);
	color = max(sceneColor, diffuse * intensity);
}
