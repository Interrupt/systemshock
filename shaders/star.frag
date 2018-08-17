#version 120

varying vec2 TexCoords;
varying float Light;

uniform sampler2D tex;

void main() {
    vec4 t = texture2D(tex, gl_PointCoord);
    vec2 mid = vec2(0.5, 0.5) - gl_PointCoord;

    float dist = 0.5 - length(mid);
    dist *= 3.0;
    dist = min(dist, 1.0);

    gl_FragColor = vec4(dist * t.r * Light, dist * t.g * Light, dist * t.b * Light, t.a);
}
