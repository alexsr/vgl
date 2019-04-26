cmake_minimum_required(VERSION 3.1...3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

set(SHADERS_BIN_PATH ${PROJECT_BINARY_DIR}/shaders)

macro(compile_to_spirv shader_path platform)
	set(compile_command glslc)
	if(IS_ABSOLUTE ${shader_path})
		log_error("Path to shader has to be relative: ${shader_path}")
	else()
		set(rel_path ${shader_path})
		get_filename_component(abs_file_path ./${shader_path} REALPATH)
	endif()
	set(out_path ${SHADERS_BIN_PATH}/./${rel_path})
	execute_process(COMMAND ${compile_command} -M "${abs_file_path}" OUTPUT_VARIABLE file_dependencies ERROR_VARIABLE file_error)
	if(file_error)
		log_error(${file_error})
		return()
	endif()
	string(REPLACE " " ";" dependency_list ${file_dependencies})
	list(REMOVE_AT dependency_list 0)
	add_custom_command(
		OUTPUT ${out_path}
		COMMAND ${compile_command} "${abs_file_path}" -o "${out_path}" --target-env=${platform}
		DEPENDS ${dependency_list})
	log_msg("Add ${platform} shader ${rel_path}")
	list(APPEND SPIRV_${platform} ${out_path})
endmacro()

macro(compile_glsl_to_spirv shader_path)
	compile_to_spirv(${shader_path} opengl)
endmacro()

macro(compile_to_spirv_all platform)
	file(GLOB files "*.frag" "*.vert" "*.geom" "*.tese" "*.tesc" "*.comp")
	foreach(file ${files})
		compile_to_spirv(${file} ${platform})
	endforeach()
endmacro()

compile_glsl_to_spirv(imgui/imgui.vert)

add_custom_target(shaders ALL DEPENDS ${SPIRV_opengl})
unset(SPIRV_opengl)