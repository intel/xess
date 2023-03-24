function(hlsl_compile TARGET_NAME SHADER_FILES SHADER_TYPE SHADER_INC_PATH SHADER_DEFINES SHADER_DEBUG)
  if (NOT (${CMAKE_GENERATOR} STREQUAL "Ninja"))
    # HLSL compiler only receive path with back-slash.
    STRING(REGEX REPLACE "/" "\\\\" SHADER_INC_PATH ${SHADER_INC_PATH})
    
    if (${SHADER_DEBUG})
        set (SHADER_FLAGS "/Vn\"g_p%(Filename)\" /I\"${SHADER_INC_PATH}\" /D \"${SHADER_DEFINES}\" /Zi /Od -Qembed_debug")
    else()
        set (SHADER_FLAGS "/Vn\"g_p%(Filename)\" /I\"${SHADER_INC_PATH}\" /D \"${SHADER_DEFINES}\"")
    endif()

    set_property(SOURCE ${SHADER_FILES} PROPERTY VS_SHADER_TYPE ${SHADER_TYPE})
    set_property(SOURCE ${SHADER_FILES} PROPERTY VS_SHADER_ENTRYPOINT main)
    set_property(SOURCE ${SHADER_FILES} PROPERTY VS_SHADER_MODEL 6.2)
    set_property(SOURCE ${SHADER_FILES} PROPERTY VS_SHADER_FLAGS ${SHADER_FLAGS})
    set_property(SOURCE ${SHADER_FILES} PROPERTY VS_SHADER_OUTPUT_HEADER_FILE "$(OutDir)CompiledShaders/%(Filename).h")
    set_property(SOURCE ${SHADER_FILES} PROPERTY VS_SHADER_OBJECT_FILE_NAME "")
  endif()
endfunction()

function(add_target_dxc_commands TARGET_NAME SHADER_FILES SHADER_INCLUDE_FILES SHADER_INC_PATH SHADER_DEFINES SHADER_DEBUG)
  STRING(REGEX REPLACE "/" "\\\\" SHADER_INC_PATH_WIN ${SHADER_INC_PATH})
    foreach(SHADER_FILE ${SHADER_FILES})
      get_filename_component(HLSL_NAME_WE ${SHADER_FILE} NAME_WE)
      get_filename_component(SHADER_FOLDER ${SHADER_FILE} DIRECTORY)
      get_source_file_property(SHADER_TYPE ${SHADER_FILE} ShaderType)

      set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/CompiledShaders)
      STRING(REGEX REPLACE "/" "\\\\" SHADER_OUTPUT_DIR_WIN ${SHADER_OUTPUT_DIR})
      STRING(REGEX REPLACE "/" "\\\\" SHADER_FILE_WIN ${SHADER_FILE})
      
      set (SHADER_FLAGS)
      if (${SHADER_DEBUG})
          set (SHADER_FLAGS /Vn\"g_p${HLSL_NAME_WE}\" /I\"${SHADER_INC_PATH_WIN}\" /D \"${SHADER_DEFINES}\" /Zi /Od -Qembed_debug)
      else()
          set (SHADER_FLAGS /Vn\"g_p${HLSL_NAME_WE}\" /I\"${SHADER_INC_PATH_WIN}\" /D \"${SHADER_DEFINES}\")
      endif()

      # Prepare output folder
      add_custom_command(TARGET ${TARGET_NAME}
      COMMAND ${CMAKE_COMMAND} -E make_directory \"${SHADER_OUTPUT_DIR}\"
      )

      # Call dxc.exe
      add_custom_command(OUTPUT ${SHADER_OUTPUT_DIR_WIN}\\${HLSL_NAME_WE}.h
      DEPENDS ${SHADER_FILE} ${SHADER_INCLUDE_FILES}
      COMMAND dxc.exe /nologo /E"main" /T ${SHADER_TYPE}_6_2 /Fh \"${SHADER_OUTPUT_DIR_WIN}\\${HLSL_NAME_WE}.h\" ${SHADER_FLAGS} \"${SHADER_FILE_WIN}\"
      MAIN_DEPENDENCY ${SHADER_FILE}
      WORKING_DIRECTORY ${SHADER_FOLDER}
      )
    endforeach(SHADER_FILE)
endfunction()