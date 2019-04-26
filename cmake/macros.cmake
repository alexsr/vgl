macro (log_msg)
	message(STATUS "${project_name}: ${ARGV}")
endmacro()

macro (log_error)
	message(SEND_ERROR "${project_name}: ${ARGV}")
endmacro()

macro (log_warning)
	message(WARNING "${project_name}: ${ARGV}")
endmacro()

# Overwrite message to still print if FATAL_ERROR is encountered
function(message)
    if (NOT MESSAGE_QUIET OR "${ARGV0}" MATCHES "FATAL_ERROR")
        _message(${ARGV})
    endif()
endfunction()
