FUNCTION(target_set_folder target folder)
  if(TARGET ${target})
    set_target_properties(${target} PROPERTIES
      FOLDER "${folder}"
    )
  endif()
ENDFUNCTION()