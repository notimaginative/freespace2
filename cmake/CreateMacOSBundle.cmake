#
# Build an app bundle on macOS.
#
# If packaging a demo, define cmake variable "DEMO_GAME_DATA" at configure time
# as the path to the demo install location for the build variant (FS1 or FS2).
# If *not* specified it will default to using "~/games/freespacedemo" or
# "~/games/freespace2demo" depending on the configuration.
#

if(NOT APPLE)
	message(WARNING "macOS bundle creation not possible!")
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

set(DIST_DIR "${CMAKE_SOURCE_DIR}/dist/macos")
set(APP_PATH "${CMAKE_BINARY_DIR}/install/$<CONFIG>/${APP_NAME}.app")

# clear existing app
install(CODE "
	if(EXISTS \"${APP_PATH}\")
		message(STATUS \"Removing existing bundle: ${APP_NAME}.app\")
		file(REMOVE_RECURSE \"${APP_PATH}\")
	endif()
")

# configure and install plist
configure_file(
	"${DIST_DIR}/Info.plist.in"
	"${CMAKE_BINARY_DIR}/Info.plist"
	@ONLY
)

install(FILES
	"${CMAKE_BINARY_DIR}/Info.plist"
	DESTINATION "${APP_PATH}/Contents"
)

# install icon
install(FILES
	"${DIST_DIR}/${ICON_FILE}.icns"
	DESTINATION "${APP_PATH}/Contents/Resources"
)

# install multi config files for PXO
if(NOT FS1)
	install(FILES
		"${CMAKE_SOURCE_DIR}/dist/pxo/multi.cfg"
		DESTINATION "${APP_PATH}/Contents/Resources/Data"
	)
else()
	install(FILES
		"${CMAKE_SOURCE_DIR}/dist/pxo/pxo.cfg"
		"${CMAKE_SOURCE_DIR}/dist/pxo/std.cfg"
		DESTINATION "${APP_PATH}/Contents/Resources/Data"
	)
endif()

# install main binaries
install(PROGRAMS
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${FS_BINARY}"
	DESTINATION "${APP_PATH}/Contents/MacOS"
)

# also install ANGLE libs
install(PROGRAMS
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libEGL.dylib"
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libGLESv2.dylib"
	DESTINATION "${APP_PATH}/Contents/Frameworks"
	OPTIONAL
)

# and do tools, if any exist
install(PROGRAMS
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ac"
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/cfileutil"
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/cryptstring"
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/nebedit"
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/pofview"
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scramble"
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fonttool"
	DESTINATION "${APP_PATH}/Contents/MacOS"
	OPTIONAL
)

# extra stuff for fonttool (if it was built)
install(FILES
	"${CMAKE_BINARY_DIR}/src/fonttool/fonttool.pcx"
	DESTINATION "${APP_PATH}/Contents/Resources/Data/Interface"
	OPTIONAL
)

# install game data for demo versions, if possible
if(DEMO)
	if(NOT DEMO_GAME_DATA)
		if(NOT FS1)
			set(DEMO_GAME_DATA "$ENV{HOME}/games/freespace2demo")
		else()
			set(DEMO_GAME_DATA "$ENV{HOME}/games/freespacedemo")
		endif()
	endif()

	if(NOT EXISTS "${DEMO_GAME_DATA}")
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
			DESTINATION "${APP_PATH}/Contents/Resources/${dir}"
		)
	endforeach()
endif()

# download and install FS2 high-quality texture pack (if enabled)
if(FS2_TEX_PACK AND NOT FS1 AND NOT DEMO)
  install(CODE "
    set(CMAKE_BINARY_DIR \"${CMAKE_BINARY_DIR}\")
    set(APP_PATH \"${APP_PATH}\")
  ")

  install(CODE [[
    message(STATUS "Downloading HQ texture pack...")

    file(DOWNLOAD
      "https://pxo.nottheeye.com/files/freespace2/hqtexpack.zip"
      "${CMAKE_BINARY_DIR}/install/hqtexpack.zip"
    )

    message(STATUS "Installing HQ texture pack...")

    file(ARCHIVE_EXTRACT
      INPUT "${CMAKE_BINARY_DIR}/install/hqtexpack.zip"
      DESTINATION "${APP_PATH}/Contents/Resources/"
    )
  ]])
endif()

# fixup bundle
install(CODE "
	include(BundleUtilities)
	fixup_bundle(\"${APP_PATH}\" \"\" \"\")
")

# finally, sign the bundle with an ad-hoc signature
install(CODE "
	execute_process(COMMAND codesign --force --timestamp --deep --sign - \"${APP_PATH}\")
")
