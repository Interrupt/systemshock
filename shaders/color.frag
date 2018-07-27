#version 110

varying vec2 TexCoords;
varying float Light;

uniform sampler2D tex;

void main() {
    vec4 t = texture2D(tex, TexCoords);
    gl_FragColor = vec4(t.r * Light, t.g * Light, t.b * Light, t.a);
}
