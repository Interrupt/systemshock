#version 110

attribute vec2 texcoords;
attribute float light;
attribute vec4 color;

varying vec2 TexCoords;
varying float Light;
varying vec4 Color;

uniform mat4 view;
uniform mat4 proj;

void main() {
    TexCoords = texcoords;
    Light = light;
    Color = color;
    gl_Position = proj * view * gl_Vertex;
}
