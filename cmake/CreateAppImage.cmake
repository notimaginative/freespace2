#
# Build an AppImage bundle on Linux.
#
# If packaging a demo, define cmake variable "DEMO_GAME_DATA" at configure time
# as the path to the demo install location for the build variant (FS1 or FS2).
# If *not* specified it will default to using "~/games/freespacedemo" or
# "~/games/freespace2demo" depending on the configuration.
#

if(NOT LINUX)
	message(WARNING "Linux AppImage creation not possible!")
	return()
endif()


# make sure that appimagetool is available
unset(APPIMAGETOOL CACHE)
find_program(APPIMAGETOOL NAMES
	appimagetool
	appimagetool.AppImage
	appimagetool-${CMAKE_HOST_SYSTEM_PROCESSOR}
	appimagetool-${CMAKE_HOST_SYSTEM_PROCESSOR}.AppImage
	HINTS "${CMAKE_BINARY_DIR}"
)

if (NOT APPIMAGETOOL)
	install(CODE "
		set(CMAKE_HOST_SYSTEM_PROCESSOR \"${CMAKE_HOST_SYSTEM_PROCESSOR}\")
		set(CMAKE_BINARY_DIR \"${CMAKE_BINARY_DIR}\")
	")

	install(CODE [[
		message(STATUS "Existing AppImage tool not found. Downloading...")

		set(TOOL_FNAME appimagetool-${CMAKE_HOST_SYSTEM_PROCESSOR}.AppImage)
		set(APPIMAGETOOL "${CMAKE_BINARY_DIR}/${TOOL_FNAME}" CACHE FILEPATH "" FORCE)

		file(DOWNLOAD
			"https://github.com/AppImage/appimagetool/releases/download/continuous/${TOOL_FNAME}"
			"${APPIMAGETOOL}"
			SHOW_PROGRESS
		)

		file(CHMOD "${APPIMAGETOOL}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
	]])
else()
	# make sure appimagetool path is set for install code
	install(CODE "
		set(APPIMAGETOOL \"${APPIMAGETOOL}\")
	")
endif()


set(APP_NAME "FreeSpace")
set(ICON_FILE "FS")
set(EXE_BINARY ${FS_BINARY})

if(NOT FS1)
	string(APPEND APP_NAME " 2") # has space
	string(APPEND ICON_FILE "2") # no space

	# from: https://wiki.hard-light.net/index.php/FreeSpace_2_game_information
	set(APP_DESCRIPTION "
		<p>
		Thirty-two years have passed since the end of the Great War, we are now
		in the year of 2367. Ten years after the destruction of the SD Lucifer
		in the Sol system, the Terran and Vasudan species forged a new alliance,
		called the Galactic Terran-Vasudan Alliance. The Reconstruction period
		began in the galaxy as both races prepare their civilisations for the
		return of the Shivans.
		</p>
		<p>
		Even though, not everything goes very well: Admiral Aken Bosch formed
		the Neo-Terra Front, a true alliance of an ideal considered hostile by
		the Galactic Terran-Vasudan Alliance. Both sides have been at war with
		each other for eighteen months now. Deneb, Alpha Centauri and Epsilon
		Pegasi are the focus points in this war.
		</p>
		<p>
		Admiral Bosch's rebellion seems to be winning.
		</p>
	")
else()
	# from: https://wiki.hard-light.net/index.php/FreeSpace_1_game_information
	set(APP_DESCRIPTION "
		<p>
		The Terrans and the Vasudans are in the middle of a high pitched,
		14 year, intergalactic war. With the front nearly at a standstill and
		the casualties ranging in the billions, things look bleak. Then, during
		the middle of the fighting, a new foe arrives, named the Shivans by
		Terran Command. The Shivans systematically wipe out Terran and Vasudan
		outposts alike. With both Terran and Vasudans on the run, they must
		work together against their common enemy if they are to save their
		people, civilization, and homeworlds.
		</p>
	")
endif()

if(DEMO)
	string(APPEND APP_NAME " Demo") # has space
endif()

# TODO: add arch to filename
set(APP_FILENAME "${APP_NAME}-${CMAKE_SYSTEM_PROCESSOR}.AppImage")
string(REPLACE " " "" APP_FILENAME "${APP_FILENAME}")

string(TOLOWER "${APP_NAME}" APP_NAME_SAFE)
string(REPLACE " " "" APP_NAME_SAFE "${APP_NAME_SAFE}")

set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install")

set(DIST_DIR "${CMAKE_SOURCE_DIR}/dist/appimage")
set(APP_PATH "${CMAKE_BINARY_DIR}/install/${APP_FILENAME}")
set(APPDIR_PATH "AppDir")


# clear existing app
install(CODE "
	if(EXISTS \"${APP_PATH}\")
		message(STATUS \"Removing existing bundle: ${APP_FILENAME}\")
		file(REMOVE_RECURSE \"${APP_PATH}\")
	endif()
	if(EXISTS \"${CMAKE_INSTALL_PREFIX}/${APPDIR_PATH}\")
		message(STATUS \"Removing existing bundle build dir\")
		file(REMOVE_RECURSE \"${CMAKE_INSTALL_PREFIX}/${APPDIR_PATH}\")
	endif()
")


# configure and install AppRun
configure_file(
	"${DIST_DIR}/AppRun.in"
	"${CMAKE_BINARY_DIR}/AppRun"
	@ONLY
)

install(PROGRAMS
	"${CMAKE_BINARY_DIR}/AppRun"
	DESTINATION "${APPDIR_PATH}"
)

# configure and install metainfo
configure_file(
	"${DIST_DIR}/game.appdata.xml.in"
	"${CMAKE_BINARY_DIR}/${APP_ID}.appdata.xml"
	@ONLY
)

install(FILES
	"${CMAKE_BINARY_DIR}/${APP_ID}.appdata.xml"
	DESTINATION "${APPDIR_PATH}/usr/share/metainfo"
)


# configure and install desktop file, and add symlink to app root
configure_file(
	"${DIST_DIR}/game.desktop.in"
	"${CMAKE_BINARY_DIR}/${APP_ID}.desktop"
	@ONLY
)

install(FILES
	"${CMAKE_BINARY_DIR}/${APP_ID}.desktop"
	DESTINATION "${APPDIR_PATH}/usr/share/applications"
)

install(CODE "
	execute_process(
		COMMAND ${CMAKE_COMMAND} -E create_symlink
		\"usr/share/applications/${APP_ID}.desktop\"
		\"${APP_ID}.desktop\"
		WORKING_DIRECTORY \"${CMAKE_INSTALL_PREFIX}/${APPDIR_PATH}\"
	)
")


# install icon, and add symlink to app root
install(FILES
	"${DIST_DIR}/${ICON_FILE}_256.png"
	DESTINATION "${APPDIR_PATH}/usr/share/icons/hicolor/256x256/apps"
	RENAME "${APP_ID}.png"
)

install(CODE "
	execute_process(
		COMMAND ${CMAKE_COMMAND} -E create_symlink
		\"usr/share/icons/hicolor/256x256/apps/${APP_ID}.png\"
		\"${APP_ID}.png\"
		WORKING_DIRECTORY \"${CMAKE_INSTALL_PREFIX}/${APPDIR_PATH}\"
	)
")


# install multi config files for PXO
if(NOT FS1)
	install(FILES
		"${CMAKE_SOURCE_DIR}/dist/pxo/multi.cfg"
		DESTINATION "${APPDIR_PATH}/opt/${APP_NAME_SAFE}/Data"
	)
else()
	install(FILES
		"${CMAKE_SOURCE_DIR}/dist/pxo/pxo.cfg"
		"${CMAKE_SOURCE_DIR}/dist/pxo/std.cfg"
		DESTINATION "${APPDIR_PATH}/opt/${APP_NAME_SAFE}/Data"
	)
endif()


# install main binaries
install(PROGRAMS
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${FS_BINARY}"
	DESTINATION "${APPDIR_PATH}/opt/${APP_NAME_SAFE}"
)

if(STANDALONE_BINARY)
	install(PROGRAMS
		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${STANDALONE_BINARY}"
		DESTINATION "${APPDIR_PATH}/opt/${APP_NAME_SAFE}"
	)
endif()


# and do tools, if any exist
install(PROGRAMS
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ac"
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/cfileutil"
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/cryptstring"
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/nebedit"
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/pofview"
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scramble"
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fonttool"
	DESTINATION "${APPDIR_PATH}/opt/${APP_NAME_SAFE}"
	OPTIONAL
)


# extra stuff for fonttool (if it was built)
install(FILES
	"${CMAKE_BINARY_DIR}/src/fonttool/fonttool.pcx"
	DESTINATION "${APPDIR_PATH}/opt/${APP_NAME_SAFE}/Data/Interface"
	OPTIONAL
)


# fixup bundle binaries
install(CODE "
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}\")
	set(BUNDLE_DIR \"${CMAKE_INSTALL_PREFIX}/${APPDIR_PATH}/opt/${APP_NAME_SAFE}\")
	set(EXE_BINARY \"${EXE_BINARY}\")
")

install(CODE [[
	include(BundleUtilities)
	fixup_bundle("${BUNDLE_DIR}/${EXE_BINARY}" "" "")

	# strip RUNPATH from binaries
	# If we let cmake do this during install then fixup_bundle() fails to copy
	# needed libs. Easier (but maybe unwise) to make use of undocumented
	# internal file() command to clear it manually, *after* fixup_bundle() is
	# done with its thing
	file(GLOB BINARIES
		LIST_DIRECTORIES false
		RELATIVE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/*"
	)

	foreach(bin ${BINARIES})
		set(bin_path "${BUNDLE_DIR}/${bin}")

		if(EXISTS "${bin_path}" AND NOT IS_SYMLINK "${bin_path}")
			file(RPATH_SET FILE "${bin_path}" NEW_RPATH "")
		endif()
	endforeach()
]])


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
			DESTINATION "${APPDIR_PATH}/opt/${APP_NAME_SAFE}/${dir}"
		)
	endforeach()
endif()


# create AppImage
install(CODE "
	set(CMAKE_SYSTEM_PROCESSOR \"${CMAKE_SYSTEM_PROCESSOR}\")
	set(CMAKE_INSTALL_PREFIX \"${CMAKE_INSTALL_PREFIX}\")
	set(APPDIR_PATH \"${APPDIR_PATH}\")
	set(APP_PATH \"${APP_PATH}\")
")

install(CODE [[
	set(ENV{ARCH} ${CMAKE_SYSTEM_PROCESSOR})
	execute_process(COMMAND
		${APPIMAGETOOL} "${CMAKE_INSTALL_PREFIX}/${APPDIR_PATH}" "${APP_PATH}"
	)
]])


# clean up temporary app_dir
install(CODE "
	if(EXISTS \"${CMAKE_INSTALL_PREFIX}/${APPDIR_PATH}\")
		message(STATUS \"Removing temporary bundle build dir\")
		file(REMOVE_RECURSE \"${CMAKE_INSTALL_PREFIX}/${APPDIR_PATH}\")
	endif()
")
