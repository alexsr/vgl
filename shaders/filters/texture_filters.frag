#version 460

layout (location = 0) in vec2 _uv;

layout (binding = 0) uniform sampler2D tex;

layout (location = 0) uniform int kernel_size;
layout (location = 1) uniform int filter_option;
layout (location = 2) uniform float strength;

layout (location = 0) out vec4 _color;

float factorial(float n) {
    float result = 1;
    for (float i = 1; i <= n; i++) {
        result *= i;
    }
    return result;
}

float binomial_denom(float n, float k) {
    return factorial(k) * factorial(n - k);
}

void main() {
    ivec2 tex_size = textureSize(tex, 0);
    vec2 uv_offset = vec2(1, 1) / vec2(tex_size);
	float half_size = (kernel_size - 1) / 2.0;
	if (kernel_size <= 0) {
        _color = texture(tex, _uv);
		return;
	}

    // box blur 1 or unsharp masking 2
    if ((filter_option == 1 || filter_option == 2)) {
        vec4 mean = vec4(0.0);
        for (float i = -half_size; i <= half_size; i++) {
            for (float j = -half_size; j <= half_size; j++) {
                mean += texture(tex, _uv + uv_offset * vec2(i, j));
            }
        }
        _color = mean / kernel_size / kernel_size;
        if (filter_option == 2) {
            vec4 f = texture(tex, _uv);
            _color = f + (f - _color) * strength;
        }
    }
    // sobel filter
    else if (filter_option == 3) {
        vec4 left = -2.0 * texture(tex, _uv + uv_offset * vec2(-1, 0));
        vec4 left_top = -texture(tex, _uv + uv_offset * vec2(-1, 1));
        vec4 left_bottom = -texture(tex, _uv + uv_offset * vec2(-1, -1));
        vec4 right = 2.0 * texture(tex, _uv + uv_offset * vec2(1, 0));
        vec4 right_top = texture(tex, _uv + uv_offset * vec2(1, 1));
        vec4 right_bottom = texture(tex, _uv + uv_offset * vec2(1, -1));
        _color = 2.0 * (left + left_top + left_bottom + right + right_top + right_bottom) / 8.0;
    }
    // median 3x3 http://pages.ripco.net/~jgamble/nw.html
    // similar to https://casual-effects.com/research/McGuire2008Median/median.pix
    #define SWAP(a, b) temp = a; a = min(a, b); b = max(temp, b);
    else if (filter_option == 4) {
        vec4 left_top = texture(tex, _uv + uv_offset * vec2(-1, 1));
        vec4 left = texture(tex, _uv + uv_offset * vec2(-1, 0));
        vec4 left_bottom = texture(tex, _uv + uv_offset * vec2(-1, -1));
        vec4 top = texture(tex, _uv + uv_offset * vec2(0, 1));
        vec4 mid = texture(tex, _uv + uv_offset * vec2(0, 0));
        vec4 bottom = texture(tex, _uv + uv_offset * vec2(0, -1));
        vec4 right_top = texture(tex, _uv + uv_offset * vec2(1, 1));
        vec4 right = texture(tex, _uv + uv_offset * vec2(1, 0));
        vec4 right_bottom = texture(tex, _uv + uv_offset * vec2(1, -1));

        vec4 temp;
        SWAP(left_top, left);
        SWAP(top, mid);
        SWAP(right_top, right);
        SWAP(left, left_bottom);
        SWAP(mid, bottom);
        SWAP(right, right_bottom);
        SWAP(left_top, left);
        SWAP(top, mid);
        SWAP(right_top, right);
        SWAP(left_top, top);
        SWAP(top, right_top);
        SWAP(left, mid);
        SWAP(mid, right);
        SWAP(left, mid);
        SWAP(left_bottom, bottom);
        SWAP(bottom, right_bottom);
        SWAP(left_bottom, bottom);
        SWAP(left_bottom, right_top);
        SWAP(mid, right_top);
        SWAP(left_bottom, mid);
        _color = mid;
    }
    // binomial filter
    else if (filter_option == 5) {
        float n = kernel_size - 1.0;
        float n_factorial = factorial(n);
        vec3 mean = vec3(0.0);
        for (int i = 0; i < kernel_size; i++) {
            for (int j = 0; j < kernel_size; j++) {
			vec2 ij = vec2(i, j) - half_size;
                mean += n_factorial / binomial_denom(n, i)
                        * n_factorial / binomial_denom(n, j)
                        * texture(tex, _uv + uv_offset * ij).rgb;
            }
        }
		float factor = pow(2.0, float(n));
        _color.rgb = mean / factor / factor;
		_color.a = 1.0f;
    }
    else {
        _color = texture(tex, _uv);
    }
}
