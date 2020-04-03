#pragma once

#include <type_traits>
#include <Eigen/Core>

namespace Eigen {
using Vector2ui = Matrix<unsigned int, 2, 1>;
using Vector3ui = Matrix<unsigned int, 3, 1>;
using Vector4ui = Matrix<unsigned int, 4, 1>;
} // namespace Eigen

namespace vgl {
/*constexpr glm::vec4 test;
template <typename>
struct is_glm_vec : std::false_type {};

template <glm::length_t L, typename T, glm::qualifier Q>
struct is_glm_vec<glm::vec<L, T, Q>> : std::true_type {};

template <typename T>
constexpr bool is_glm_vec_v = is_glm_vec<T>::value;*/
}
