cmake_minimum_required(VERSION 3.1...3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

function (add_dependency_subdir dir)
	set(MESSAGE_QUIET ON)
		add_subdirectory(${EXTERNAL_PATH}/${dir})
	unset(MESSAGE_QUIET)
	log_msg("Add external dependency ${dir}")
endfunction()

function (find_ext_package ext_name option)
	set(MESSAGE_QUIET ON)
		find_package(${ext_name} ${option})
	unset(MESSAGE_QUIET)
	log_msg("Add package ${ext_name}")
endfunction()

function (add_header_only_lib root libname require)
	if (EXISTS ${root})
		add_library(${libname}::${libname} INTERFACE IMPORTED)
		target_include_directories(${libname}::${libname} INTERFACE ${root})

		target_compile_features(${libname}::${libname} INTERFACE cxx_std_17)
		log_msg("Include ${libname}")
	else()
		if (${require} EQUAL "REQUIRED")
			log_error("${libname} not found")
		else()
			log_warning("${libname} not found")
		endif()
	endif()
endfunction()

find_ext_package(openmp REQUIRED)
find_ext_package(OpenGL REQUIRED)
find_ext_package(nativefiledialog REQUIRED)

include(${CMAKE_MODULE_PATH}/imgui.cmake)

set(BUILD_SHARED_LIBS OFF)
find_ext_package(glad REQUIRED)

set(GLM_TEST_ENABLE OFF)
add_dependency_subdir(glm)
add_library(glm::glm ALIAS glm)

add_header_only_lib(${EXTERNAL_PATH}/stb stb REQUIRED)
add_header_only_lib(${EXTERNAL_PATH}/catch2/single_include/catch2 catch2 REQUIRED)

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
add_dependency_subdir(glfw)
add_library(glfw::glfw ALIAS glfw)

set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_NO_EXPORT ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(ASSIMP_OPT_BUILD_PACKAGES ON)
set(ASSIMP_BUILD_AMF_IMPORTER OFF)
set(ASSIMP_BUILD_AC_IMPORTER OFF)
set(ASSIMP_BUILD_ASE_IMPORTER OFF)
set(ASSIMP_BUILD_ASSBIN_IMPORTER OFF)
set(ASSIMP_BUILD_ASSXML_IMPORTER OFF)
set(ASSIMP_BUILD_B3D_IMPORTER OFF)
set(ASSIMP_BUILD_BVH_IMPORTER OFF)
set(ASSIMP_BUILD_DXF_IMPORTER OFF)
set(ASSIMP_BUILD_CSM_IMPORTER OFF)
set(ASSIMP_BUILD_HMP_IMPORTER OFF)
set(ASSIMP_BUILD_IRRMESH_IMPORTER OFF)
set(ASSIMP_BUILD_IRR_IMPORTER OFF)
set(ASSIMP_BUILD_LWO_IMPORTER OFF)
set(ASSIMP_BUILD_LWS_IMPORTER OFF)
set(ASSIMP_BUILD_MD2_IMPORTER OFF)
set(ASSIMP_BUILD_MD3_IMPORTER OFF)
set(ASSIMP_BUILD_MD5_IMPORTER OFF)
set(ASSIMP_BUILD_MDC_IMPORTER OFF)
set(ASSIMP_BUILD_MDL_IMPORTER OFF)
set(ASSIMP_BUILD_NFF_IMPORTER OFF)
set(ASSIMP_BUILD_NDO_IMPORTER OFF)
set(ASSIMP_BUILD_OFF_IMPORTER OFF)
set(ASSIMP_BUILD_OGRE_IMPORTER OFF)
set(ASSIMP_BUILD_OPENGEX_IMPORTER OFF)
set(ASSIMP_BUILD_COB_IMPORTER OFF)
set(ASSIMP_BUILD_IFC_IMPORTER OFF)
set(ASSIMP_BUILD_XGL_IMPORTER OFF)
set(ASSIMP_BUILD_Q3D_IMPORTER OFF)
set(ASSIMP_BUILD_Q3BSP_IMPORTER OFF)
set(ASSIMP_BUILD_RAW_IMPORTER OFF)
set(ASSIMP_BUILD_SIB_IMPORTER OFF)
set(ASSIMP_BUILD_SMD_IMPORTER OFF)
set(ASSIMP_BUILD_TERRAGEN_IMPORTER OFF)
set(ASSIMP_BUILD_3D_IMPORTER OFF)
set(ASSIMP_BUILD_X_IMPORTER OFF)
set(ASSIMP_BUILD_X3D_IMPORTER OFF)
set(ASSIMP_BUILD_3MF_IMPORTER OFF)
set(ASSIMP_BUILD_MMD_IMPORTER OFF)
set(BUILD_SHARED_LIBS ON)

set(CURRENT_BUILD_TYPE ${CMAKE_BUILD_TYPE})
set(CMAKE_BUILD_TYPE "Release")
add_dependency_subdir(assimp)
add_library(assimp::assimp ALIAS assimp)
target_compile_options(assimp PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)
set(CMAKE_BUILD_TYPE ${CURRENT_BUILD_TYPE})
set(BUILD_SHARED_LIBS OFF)
