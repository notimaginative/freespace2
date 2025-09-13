#
# Create a distributable archive on Windows.
#
# If packaging a demo, define cmake variable "DEMO_GAME_DATA" at configure time
# as the path to the demo install location for the build variant (FS1 or FS2).
#

if(NOT WIN32)
	message(WARNING "Windows archive creation not possible!")
	return()
endif()

set(APP_NAME "FreeSpace")
set(ICON_FILE "FS")
set(EXE_BINARY ${FS_BINARY})

if(NOT FS1)
	string(APPEND APP_NAME " 2") # has space
	string(APPEND ICON_FILE "2") # no space
endif()

if(DEMO)
	string(APPEND APP_NAME " Demo") # has space
endif()

set(DIST_DIR "${CMAKE_SOURCE_DIR}/dist/windows")
set(APP_PATH "${CMAKE_BINARY_DIR}/install/${APP_NAME}")
set(ZIP_PATH "${CMAKE_BINARY_DIR}/install/${APP_NAME}.zip")

# clear existing app
install(CODE "
	if(EXISTS \"${APP_PATH}\")
		message(STATUS \"Removing existing app folder: ${APP_NAME}\")
		file(REMOVE_RECURSE \"${APP_PATH}\")
	endif()

	if(EXISTS \"${ZIP_PATH}\")
	    message(STATUS \"Removing existing archive file: ${APP_NAME}.zip\")
		file(REMOVE \"${ZIP_PATH}\")
	endif()
")

# install multi config files for PXO
if(NOT FS1)
	install(FILES
		"${CMAKE_SOURCE_DIR}/dist/pxo/multi.cfg"
		DESTINATION "${APP_PATH}/Data"
	)
else()
	install(FILES
		"${CMAKE_SOURCE_DIR}/dist/pxo/pxo.cfg"
		"${CMAKE_SOURCE_DIR}/dist/pxo/std.cfg"
		DESTINATION "${APP_PATH}/Data"
	)
endif()

# install dlls
install(FILES
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/OpenAL32.dll"
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/SDL3.dll"
	DESTINATION "${APP_PATH}"
)

# install main binaries
install(PROGRAMS
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${FS_BINARY}.exe"
	DESTINATION "${APP_PATH}"
)

# and do tools, if any exist
install(PROGRAMS
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ac.exe"
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/cfileutil.exe"
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/cryptstring.exe"
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/nebedit.exe"
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/pofview.exe"
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scramble.exe"
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fonttool.exe"
	DESTINATION "${APP_PATH}"
	OPTIONAL
)

# extra stuff for fonttool (if it was built)
install(FILES
	"${CMAKE_BINARY_DIR}/src/fonttool/fonttool.pcx"
	DESTINATION "${APP_PATH}/Data/Interface"
	OPTIONAL
)

# install game data for demo versions, if possible
if(DEMO)
	if(NOT DEMO_GAME_DATA OR NOT EXISTS "${DEMO_GAME_DATA}")
		message(WARNING "Demo game data path not found!")
		message(WARNING "Proper demo packaging will not be possible!")
		message(WARNING "Please check value of DEMO_GAME_DATA")
	endif()

	file(GLOB DEMO_FILES
		RELATIVE "${DEMO_GAME_DATA}"
		"${DEMO_GAME_DATA}/*.[vV][pP]"
		"${DEMO_GAME_DATA}/[dD]ata/*.[vV][pP]"
		"${DEMO_GAME_DATA}/*.[tT][xX][tT]"
		"${DEMO_GAME_DATA}/*.[rR][tT][fF]"
	)

	foreach(item ${DEMO_FILES})
		get_filename_component(dir "${item}" DIRECTORY)

		install(FILES
			"${DEMO_GAME_DATA}/${item}"
			DESTINATION "${APP_PATH}/${dir}"
		)
	endforeach()
endif()

# create ZIP archive
install(CODE "
	message(CHECK_START \"Creating archive\")
	file(ARCHIVE_CREATE
		OUTPUT \"${APP_NAME}.zip\"
		PATHS \"${APP_NAME}\"
		FORMAT \"zip\"
		WORKING_DIRECTORY \"${CMAKE_BINARY_DIR}/install\"
	)
	message(CHECK_PASS \"done\")
")

# clean up temporary app_dir
install(CODE "
	if(EXISTS \"${APP_PATH}\")
		message(STATUS \"Removing temporary app dir\")
		file(REMOVE_RECURSE \"${APP_PATH}\")
	endif()
")
