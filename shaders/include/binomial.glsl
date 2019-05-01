#ifndef BINOMIAL_GLSL
#define BINOMIAL_GLSL

float factorial(float n) {
    float result = 1;
    for (float i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

float falling_factorial(float n, float k) {
    float result = 1;
    for (float i = n; i > n - k; i--) {
        result *= i;
    }
    return result;
}

float binomial(float n, float k) {
    return falling_factorial(n, k) / factorial(k);
}

#endif // BINOMIAL_GLSL
