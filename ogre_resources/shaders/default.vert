#version 330

attribute vec4 vertex;
uniform mat4 worldViewProj;

out vec3 color1;

//varying vec2 oUv0;

void main() {
    gl_Position = worldViewProj * vertex;
    color1 = vec3(1, 1, 1);

    //vec2 inPos = sign(vertex.xy);
    //oUv0 = (vec2(inPos.x, -inPos.y) + 1.0) * 0.5;
}
