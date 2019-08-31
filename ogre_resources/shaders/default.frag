#version 330

in vec3 color;
in vec2 tex_uv;

uniform sampler2D texture;

void main() {
    gl_FragColor = vec4(color, 1) * texture2D(texture, tex_uv);
}
