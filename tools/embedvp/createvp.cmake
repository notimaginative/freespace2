
# clean temp root
file(REMOVE_RECURSE "${EMBED_ROOT}")


# copy all needed files to temporary embed_root
file(COPY "${DIST_DIR}/embed_root"
	DESTINATION "${EMBED_ROOT}/../"
	PATTERN ".DS_Store" EXCLUDE
)

set(APP_ICON "${DIST_DIR}/images/FS")

if(NOT FS1)
	string(APPEND APP_ICON "2")
endif()

file(COPY_FILE "${APP_ICON}.png" "${EMBED_ROOT}/app_icon.png")

# bundle standalone web ui into a zip file and add to embed root
file(GLOB standalone-web RELATIVE "${DIST_DIR}/standalone-web"
	"${DIST_DIR}/standalone-web/*"
)

list(REMOVE_ITEM standalone-web ".DS_Store")

execute_process(
	COMMAND ${CMAKE_COMMAND} -E tar c
		"${EMBED_ROOT}/standalone-web.zip"
		--format=zip
		${standalone-web}
	WORKING_DIRECTORY
		"${DIST_DIR}/standalone-web"
)

# create VP archive
execute_process(
	COMMAND ${CREATEVP}
		${EMBED_ROOT}
		${EMBED_FILE}
	# set working directory to runtime output directory (for SDL3 dll on Win)
	WORKING_DIRECTORY
		${RUNTIME_DIR}
	RESULT_VARIABLE status
)

if(status)
	message("createvp error: ${status}")
endif()
