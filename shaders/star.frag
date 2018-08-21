#version 120

varying float Light;

void main() {
    vec2 mid = vec2(0.5, 0.5) - gl_PointCoord;

    float dist = 0.5 - length(mid);
    dist *= 3.0;
    dist = min(dist, 1.0);

    gl_FragColor = vec4(dist * Light, dist * Light, dist * Light, 1.0);
}
