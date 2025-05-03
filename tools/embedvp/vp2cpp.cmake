
if(NOT SOURCE_DIR OR NOT BINARY_DIR)
	mesasge(FATAL_ERROR "SOURCE_DIR and BINARY_DIR must be defined!")
endif()

if(NOT EMBED_FILE)
	mesasge(FATAL_ERROR "EMBED_FILE must be defined!")
endif()

set(EMBED_HEX)	# variable used in configure_file

file(READ ${EMBED_FILE} content HEX)
string(REGEX MATCHALL "([A-Fa-f0-9][A-Fa-f0-9])" SEPARATED_HEX ${content})

set(counter 0)
foreach(hex IN LISTS SEPARATED_HEX)
	string(APPEND EMBED_HEX " 0x${hex},")
	MATH(EXPR counter "${counter}+1")
	if(counter GREATER 11)
		string(APPEND EMBED_HEX "\n ")
		set(counter 0)
	endif()
endforeach()

configure_file(${SOURCE_DIR}/embedvp.h.in ${BINARY_DIR}/embedvp.h @ONLY)
configure_file(${SOURCE_DIR}/embedvp.cpp.in ${BINARY_DIR}/embedvp.cpp @ONLY)
