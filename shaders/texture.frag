#version 110

// The texture alpha value has two separate functions in this shader:
// - Pixels with a zero alpha value are transparent.
// - Pixels with non-zero alpha are opaque, and the alpha value
//   defines a minimum light level.

varying vec2 TexCoords;
varying float Light;

uniform sampler2D tex;

void main() {
    vec4 t = texture2D(tex, TexCoords);
    float light = max(Light, t.a);
    float alpha = t.a < 0.0001 ? 0.0 : 1.0;
    gl_FragColor = vec4(t.r * light, t.g * light, t.b * light, alpha);
}
