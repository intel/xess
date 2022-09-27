function(hlsl_compile SHADER_FILES SHADER_TYPE SHADER_INC_PATH SHADER_DEFINES SHADER_DEBUG)
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
endfunction()


macro(set_msvc_precompiled_header PCH_HEADER PCH_CREATE_SOURCE_FILES PCH_USE_SOURCE_FILES)
    if (MSVC)
      set_source_files_properties(${PCH_USE_SOURCE_FILES} PROPERTIES COMPILE_FLAGS "/Yu\"${PCH_HEADER}\" /FI\"${PCH_HEADER}\"")
      set_source_files_properties(${PCH_CREATE_SOURCE_FILES} PROPERTIES COMPILE_FLAGS "/Yc\"${PCH_HEADER}\"")
    endif()
endmacro(set_msvc_precompiled_header)

