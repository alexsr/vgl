#ifndef COLOR_CONVERSION_GLSL
#define COLOR_CONVERSION_GLSL

vec3 hsv_to_rgb(vec3 hsv) {
    float c = hsv.y * hsv.z;
    float h = hsv.x / 60.0;
    if(h >= 6.0) {
        h = 0.0;
    }
    float fract_h = fract(h);
    float p = hsv.z * (1.0 - hsv.y);
    float q = hsv.z * (1.0 - (hsv.y * fract_h));
    float t = hsv.z * (1.0 - (hsv.y * (1.0 - fract_h)));
    if (h < 0 || isnan(h)) {
        return vec3(hsv.z);
    }
    else if (h < 1) {
        return vec3(hsv.z, t, p);
    }
    else if (h < 2) {
        return vec3(q, hsv.z, p);
    }
    else if (h < 3) {
        return vec3(p, hsv.z, t);
    }
    else if (h < 4) {
        return vec3(p, q, hsv.z);
    }
    else if (h < 5) {
        return vec3(t, p, hsv.z);
    }
    else {
        return vec3(hsv.z, p, q);
    }
}

vec3 hsv2rgb(vec3 in_v)
{
    float      hh, p, q, t, ff;
    int        i;
    vec3         out_v;

    if(in_v.y <= 0.0) {       // < is bogus, just shuts up warnings
        out_v.r = in_v.z;
        out_v.g = in_v.z;
        out_v.b = in_v.z;
        return out_v;
    }
    hh = in_v.x;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = int(hh);
    ff = hh - i;
    p = in_v.z * (1.0 - in_v.y);
    q = in_v.z * (1.0 - (in_v.y * ff));
    t = in_v.z * (1.0 - (in_v.y * (1.0 - ff)));

    switch(i) {
    case 0:
        out_v.r = in_v.z;
        out_v.g = t;
        out_v.b = p;
        break;
    case 1:
        out_v.r = q;
        out_v.g = in_v.z;
        out_v.b = p;
        break;
    case 2:
        out_v.r = p;
        out_v.g = in_v.z;
        out_v.b = t;
        break;

    case 3:
        out_v.r = p;
        out_v.g = q;
        out_v.b = in_v.z;
        break;
    case 4:
        out_v.r = t;
        out_v.g = p;
        out_v.b = in_v.z;
        break;
    case 5:
    default:
        out_v.r = in_v.z;
        out_v.g = p;
        out_v.b = q;
        break;
    }
    return out_v;     
}


vec3 rgb_to_hsv(vec3 rgb) {  
    vec3 hsv = vec3(0.0);
    float max_v = max(rgb.r, max(rgb.g, rgb.b));
    float min_v = min(rgb.r, min(rgb.g, rgb.b));
    hsv.b = max_v;
    float delta = max_v - min_v;
    if (delta < 0.00001 || max_v <= 0.0) {
        return hsv;
    }
    hsv.g = delta / max_v;    
    if (rgb.r >= max_v) {
        hsv.r = (rgb.g - rgb.b) / delta;
    }
    else if (rgb.g >= max_v) {
        hsv.r = 2.0 + (rgb.b - rgb.r) / delta;
    }
    else if (rgb.b >= max_v) {
        hsv.r = 4.0 + (rgb.r - rgb.g) / delta;
    }
    hsv.r *= 60.0;
    if (hsv.r < 0) {
        hsv.r += 360.0;
    }
    return hsv;
}

vec3 rgb2hsv(vec3 in_v)
{
    vec3         out_v;
    float      min_v, max_v, delta;

    min_v = in_v.r < in_v.g ? in_v.r : in_v.g;
    min_v = min_v  < in_v.b ? min_v  : in_v.b;

    max_v = in_v.r > in_v.g ? in_v.r : in_v.g;
    max_v = max_v  > in_v.b ? max_v  : in_v.b;

    out_v.z = max_v;                                // v
    delta = max_v - min_v;
    if (delta < 0.00001)
    {
        out_v.y = 0;
        out_v.x = 0; // undefin_ved, maybe nan?
        return out_v;
    }
    if( max_v > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        out_v.y = (delta / max_v);                  // s
    } else {
        // if max is 0, then r = g = b = 0              
        // s = 0, h is undefined
        out_v.y = 0.0;
        out_v.x = 0.0;                            // its now undefined
        return out_v;
    }
    if( in_v.r >= max_v )                           // > is bogus, just keeps compilor happy
        out_v.x = ( in_v.g - in_v.b ) / delta;        // between yellow & magenta
    else
    if( in_v.g >= max_v )
        out_v.x = 2.0 + ( in_v.b - in_v.r ) / delta;  // between cyan & yellow
    else
        out_v.x = 4.0 + ( in_v.r - in_v.g ) / delta;  // between magenta & cyan

    out_v.x *= 60.0;                              // degrees

    if( out_v.x < 0.0 )
        out_v.x += 360.0;

    return out_v;
}

#endif // COLOR_CONVERSION_GLSL
