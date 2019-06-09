#version 460

layout (location = 0) in vec4 _color;

layout (location = 0) out vec4 frag_color;

void main() {
    vec2 diff = vec2(gl_PointCoord-0.5f)*2.0f;
    float d_sqrd = 1.0 - (dot(diff, diff));
    if (_color.w <= 0.0 || d_sqrd < 0.0) {
        discard;
    }
    frag_color.rgb = mix(_color.rgb, vec3(1, 1, 1), pow(_color.w, 1.0 / 3.0) - 0.01);
    float low = 0.99;
    float t = d_sqrd * d_sqrd * (3.0 - 2.0 * d_sqrd);
    frag_color.a = t * clamp(_color.w, 0, 1);
    // frag_color.rgb = mix(vec3(1), _color.rgb, atan(length(_ppos)));
}
