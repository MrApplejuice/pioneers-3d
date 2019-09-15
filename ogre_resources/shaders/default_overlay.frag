#version 330

in vec3 color;
in vec2 tex_uv;
in vec2 overlay_uv;

uniform sampler2D texture;
uniform sampler2D overlay;

void main() {
	vec4 textureColor = texture2D(texture, tex_uv);
	vec4 overlayColor = texture2D(overlay, overlay_uv);
    gl_FragColor = vec4(color, 1) * vec4(
			mix(vec3(textureColor), vec3(overlayColor), overlayColor.a), 
			textureColor.a);
}
