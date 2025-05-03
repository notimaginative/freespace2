
# clean temp root
file(REMOVE_RECURSE "${EMBED_ROOT}")


# copy all needed files to temporary embed_root
file(COPY "${DIST_DIR}/embed_root" DESTINATION "${EMBED_ROOT}/../")

set(APP_ICON "${DIST_DIR}/images/FS")

if(NOT FS1)
	string(APPEND APP_ICON "2")
endif()

file(COPY_FILE "${APP_ICON}.png" "${EMBED_ROOT}/app_icon.png")


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
