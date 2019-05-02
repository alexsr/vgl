#ifndef EIGEN_GLSL
#define EIGEN_GLSL

vec2 eigenvalues(mat2 a) {
    float p_2 = a[0][0] - a[1][1] / 2.0;
    float q = a[0][0] * a[1][1] - a[1][0] * a[0][1];
    float root = p_2 * p_2 - q;
    if (root < 0) {
        root = 0;
    }
    else {
        root = sqrt(root);
    }
    return vec2(-p_2 + root, -p_2 - root);
}

#endif // EIGEN_GLSL
