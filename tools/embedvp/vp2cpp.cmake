
if(NOT SOURCE_DIR OR NOT BINARY_DIR)
	mesasge(FATAL_ERROR "SOURCE_DIR and BINARY_DIR must be defined!")
endif()

if(NOT EMBED_FILE)
	mesasge(FATAL_ERROR "EMBED_FILE must be defined!")
endif()

set(EMBED_HEX)	# variable used in configure_file

file(READ ${EMBED_FILE} hexString HEX)

# wrap at column 32 (16 bytes)
string(REPEAT "[0-9a-f]" 32 column_pattern)
string(REGEX REPLACE "(${column_pattern})" "\\1\n" arrayValues "${hexString}")

# adds '0x' prefix and comma suffix before and after every byte respectively
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " arrayValues ${arrayValues})
# removes trailing comma
string(REGEX REPLACE ", $" "" EMBED_HEX ${arrayValues})

configure_file(${SOURCE_DIR}/embedvp.h.in ${BINARY_DIR}/embedvp.h @ONLY)
configure_file(${SOURCE_DIR}/embedvp.cpp.in ${BINARY_DIR}/embedvp.cpp @ONLY)
