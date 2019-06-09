#ifndef COLOR_CONVERSION_GLSL
#define COLOR_CONVERSION_GLSL

vec3 hsv_to_rgb(vec3 hsv) {
    float c = hsv.y * hsv.z;
    float h = hsv.x / 60.0;
    float x = c * (1.0 - abs(mod(h, 2.0) - 1.0));
    float m = hsv.z - c;
    vec3 res = vec3(m, m, m);
    if (h < 0 || isnan(h)) {
        return res;
    }
    else if (h < 1) {
        res += vec3(c, x, 0);
    }
    else if (h < 2) {
        res += vec3(x, c, 0);
    }
    else if (h < 3) {
        res += vec3(0, c, x);
    }
    else if (h < 4) {
        res += vec3(0, x, c);
    }
    else if (h < 5) {
        res += vec3(x, 0, c);
    }
    else if (h < 6) {
        res += vec3(c, 0, x);
    }
    return res;
}

#endif // COLOR_CONVERSION_GLSL
