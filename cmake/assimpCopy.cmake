FUNCTION(ASSIMP_COPY_DEBUG ProjectName)
    add_custom_command(TARGET ${ProjectName} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EXTERNAL_PATH}/assimp/lib/assimp-vc140-mtd.dll
		$<TARGET_FILE_DIR:${ProjectName}>/assimp-vc140-mtd.dll
    COMMENT "Copying Assimp binaries to '$<TARGET_FILE_DIR:${ProjectName}>'" VERBATIM)
ENDFUNCTION(ASSIMP_COPY_DEBUG)

FUNCTION(ASSIMP_COPY_RELEASE ProjectName)
    add_custom_command(TARGET ${ProjectName} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EXTERNAL_PATH}/assimp/lib/assimp-vc140-mt.dll
		$<TARGET_FILE_DIR:${ProjectName}>/assimp-vc140-mt.dll
    COMMENT "Copying Assimp binaries to '$<TARGET_FILE_DIR:${ProjectName}>'" VERBATIM)
ENDFUNCTION(ASSIMP_COPY_RELEASE)
