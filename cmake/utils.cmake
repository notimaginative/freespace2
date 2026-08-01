FUNCTION(target_set_folder target folder)
  if(TARGET ${target})
    set_target_properties(${target} PROPERTIES
      FOLDER "${folder}"
    )
  endif()
ENDFUNCTION()

FUNCTION(generate_scheme target)
  if(TARGET ${target})
    set_target_properties(${target} PROPERTIES
      XCODE_GENERATE_SCHEME ON
    )
  endif()
ENDFUNCTION()
