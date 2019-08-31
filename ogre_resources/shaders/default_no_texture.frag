#version 330

in vec3 color;
in vec2 tex_uv;

void main() {
    gl_FragColor = vec4(color, 1);
}
