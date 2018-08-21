#version 110

varying vec4 Color;
varying float Light;

void main() {
    gl_FragColor = vec4(Color.r * Light, Color.g * Light, Color.b * Light, Color.a);
}
