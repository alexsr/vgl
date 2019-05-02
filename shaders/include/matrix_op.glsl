#ifndef MATRIX_OP_GLSL
#define MATRIX_OP_GLSL

float trace(mat2 a) {
    return a[0][0] + a[1][1];
}

float trace(mat3 a) {
    return a[0][0] + a[1][1] + a[2][2];
}

float trace(mat4 a) {
    return a[0][0] + a[1][1] + a[2][2] + a[3][3];
}

float det(mat2 a) {
    return a[0][0] * a[1][1] - a[1][0] * a[0][1];
}

float det(mat3 a) {
    return a[0][0] * a[1][1] * a[2][2] + a[1][0] * a[2][1] * a[0][2]
           + a[2][0] * a[0][1] * a[1][2] - a[2][0] * a[1][1] * a[0][2]
           - a[1][0] * a[0][1] * a[2][2] - a[0][0] * a[2][1] * a[1][2];
}

float det(mat4 a) {
    return a[0][0] * det(mat3(a[1].yzw, a[2].yzw, a[3].yzw))
           - a[1][0] * det(mat3(a[0].yzw, a[2].yzw, a[3].yzw))
           + a[2][0] * det(mat3(a[0].yzw, a[1].yzw, a[3].yzw))
           - a[3][0] * det(mat3(a[0].yzw, a[1].yzw, a[2].yzw));
}

#endif // MATRIX_OP_GLSL
