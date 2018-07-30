#version 110

varying vec2 TexCoords;
varying float Light;

uniform sampler2D tex;

void main() {
    vec4 t = texture2D(tex, TexCoords);

    // Throw away emissive part for now
    float alpha = t.a > 0.5 ? 1.0 : t.a * 2.0;

    gl_FragColor = vec4(t.r * Light, t.g * Light, t.b * Light, alpha);
}
