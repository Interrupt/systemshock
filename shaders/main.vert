#version 110

attribute vec2 texcoords;
attribute float light;

varying vec2 TexCoords;
varying float Light;

uniform mat4 view;
uniform mat4 proj;

void main() {
    TexCoords = texcoords;
    Light = light;
    gl_Position = proj * view * gl_Vertex;
}
