macro(CreateSourceGroups)
  foreach(F ${ARGN})
	get_filename_component(PARENT_DIR "${F}" PATH)

	# remove absolute path and change /'s to \\'s
	string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" GROUP "${PARENT_DIR}")
	string(REPLACE "/" "\\" GROUP "${GROUP}")

	# group into "Source Files" or "Header Files"
	if (${GROUP} MATCHES "^src")
	  string(REGEX REPLACE "^src\\\\" "" GROUP "${GROUP}")
	  set(GROUP "Source Files\\${GROUP}")
	elseif (${GROUP} MATCHES "^include")
	  string(REGEX REPLACE "^include\\\\" "" GROUP "${GROUP}")
	  set(GROUP "Header Files\\${GROUP}")
	endif()

	source_group("${GROUP}" FILES "${F}")
  endforeach()
endmacro()
