#version 110

// The texture alpha value has two separate functions in this shader:
// - Pixels with a zero alpha value are transparent.
// - Pixels with non-zero alpha are opaque, and the alpha value
//   defines a minimum light level.

varying vec2 TexCoords;
varying float Light;

uniform sampler2D tex;
uniform bool nightsight;

void main() {
    vec4 t = texture2D(tex, TexCoords);

    // Alpha values > 0.5 are emissive
    float alpha = t.a > 0.5 ? 1.0 : t.a * 2.0;

    if (nightsight) {
        // approximate the blue tint of palette colors 0xb0..0xbf
        float gray = 0.40 * t.r + 0.59 * t.g + 0.11 * t.b;
        float rg = 0.7 * gray;
        float b = 0.75 * gray + 0.1;
        gl_FragColor = vec4(rg, rg, b, alpha);
    } else {
        float emissive = t.a > 0.5 ? 1.0 : 0.0;
        float light = max(Light, emissive);
        gl_FragColor = vec4(t.r * light, t.g * light, t.b * light, alpha);
    }
}
