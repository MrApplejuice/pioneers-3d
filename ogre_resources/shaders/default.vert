#version 330

attribute vec4 vertex;
attribute vec4 normal;
attribute vec4 uv0;
attribute vec4 uv1;

uniform mat4 worldMatrix;
uniform mat4 viewProjMatrix;

uniform vec3 sceneColor;
uniform vec3 diffuse;

uniform vec3 lightPos;
uniform vec4 lightAttenuation;

uniform vec2 overlay_offset;

out vec3 color;
out vec2 tex_uv;
out vec2 overlay_uv;

void main() {
	gl_Position = worldMatrix * vertex;
	vec3 worldNormal = normalize(mat3(worldMatrix) * vec3(normal));
	vec3 lposfid = lightPos - vec3(gl_Position);
	float intensity = length(lposfid);
	intensity =
			step(intensity, lightAttenuation[0]) * lightAttenuation[1]
			+ max(0, 1 - intensity / lightAttenuation[0]) * lightAttenuation[2]
			+ pow(max(0, 1 - intensity / lightAttenuation[0]), 2) * lightAttenuation[3];
	intensity = max(0, dot(normalize(worldNormal), normalize(lposfid))) * intensity;
	gl_Position = viewProjMatrix * gl_Position;

	tex_uv = vec2(uv0);
	overlay_uv = vec2(uv1) + overlay_offset;
	color = max(sceneColor, diffuse * intensity);
}
