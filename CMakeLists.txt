
project(freespace2)

cmake_minimum_required(VERSION 2.8)
cmake_policy(VERSION 2.8)

if(POLICY CMP0043)
  cmake_policy(SET CMP0043 OLD)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

include(CreateSourceGroups)


# ##############################################################################

option(FS1 "Build original FreeSpace" OFF)
option(DEMO "Create demo build" OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()


if(FS1)
  add_definitions(-DMAKE_FS1)

  if(DEMO)
	add_definitions(-DDEMO -DFS1_DEMO)
  endif()
else()
  if(DEMO)
	add_definitions(-DFS2_DEMO)
  endif()
endif()

if(NOT WIN32)
  set(PLATFORM_LIBRARIES "")

  add_definitions(-DPLAT_UNIX)
  add_definitions(-Wall)
  add_definitions(-Wshadow)
# add_definitions(-Wno-format-y2k)
# add_definitions(-Wno-deprecated)
  add_definitions(-fsigned-char)

  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DNDEBUG -DRELEASE_REAL -g -O2")
  set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -DNDEBUG -Os")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -DRELEASE_REAL -O2")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3")
else()
  set(SDL2DIR "${CMAKE_SOURCE_DIR}/../Support/SDL2" CACHE PATH "Path to SDL2")
  set(OPENALDIR "${CMAKE_SOURCE_DIR}/../Support/OpenAL" CACHE PATH "Path to OpenAL")
  set(LWSDIR "${CMAKE_SOURCE_DIR}/../Support/libwebsockets" CACHE PATH "Path to libwebsockets")
  set(WXWIDGETSDIR "${CMAKE_SOURCE_DIR}/../Support/wxWidgets" CACHE PATH "Path to wxWidgets")
  set(ANGLEDIR "${CMAKE_SOURCE_DIR}/../Support/ANGLE" CACHE PATH "Path to ANGLE")

  set(PLATFORM_LIBRARIES
	ws2_32.lib
	${ANGLEDIR}/lib/libEGL.lib
	${ANGLEDIR}/lib/libGLESv2.lib
  )

  add_definitions(/D _CRT_SECURE_NO_WARNINGS)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4100") # unreferenced parameter
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4127") # conditional expression is constant: do { } while (0)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4996") # deprecated functions: fopen, fileno, ...

  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT")
  set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MT")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()

set(BIN_SUFFIX "")

if(NOT FS1)
	set(BIN_SUFFIX "2")
endif()

if(DEMO)
	set(BIN_SUFFIX "${BIN_SUFFIX}demo")
endif()

find_package(SDL2 "2.0.5" REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenAL REQUIRED)
find_package(LibWebSockets REQUIRED)

find_package(wxWidgets COMPONENTS core base gl net)
include(${wxWidgets_USE_FILE})

# ##############################################################################


include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${CMAKE_BINARY_DIR}/include
  ${SDL2_INCLUDE_DIR}
  ${OPENAL_INCLUDE_DIR}
  ${LIBWEBSOCKETS_INCLUDE_DIR}
  ${wxWidgets_INCLUDE_DIRS}
)


add_subdirectory(src)
add_subdirectory(include)


# ##############################################################################
#

#
# custom target for source tree version info
#

add_custom_target(buildver
  COMMAND ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DBINARY_DIR=${CMAKE_BINARY_DIR}
    -P ${CMAKE_SOURCE_DIR}/cmake/GitInfo.cmake
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  SOURCES ${CMAKE_SOURCE_DIR}/cmake/GitInfo.cmake
)

#
# main code/game library
#

add_library(code
  STATIC
  ${code_SOURCE}
  ${code_HEADERS}
  ${platform_SOURCE}
  ${platform_HEADERS}
)

CreateSourceGroups(${code_SOURCE} ${platform_SOURCE})

add_dependencies(code
  buildver
)

#
# the game itself
#

set(FS_BINARY "fs${BIN_SUFFIX}")

add_executable(${FS_BINARY}
  WIN32
  ${freespace_SOURCE}
  ${freespace_HEADERS}
)

CreateSourceGroups(${freespace_SOURCE})

target_link_libraries(
  ${FS_BINARY}
  code
  ${SDL2_LIBRARY}
  ${OPENGL_LIBRARIES}
  ${OPENAL_LIBRARY}
  ${LIBWEBSOCKETS_LIBRARIES}
  ${PLATFORM_LIBRARIES}
)

if(FS1)
  set_target_properties(${FS_BINARY} PROPERTIES PROJECT_LABEL "Freespace")
else()
  set_target_properties(${FS_BINARY} PROPERTIES PROJECT_LABEL "Freespace2")
endif()

#
# launcher
#

set(LAUNCHER_BINARY "freespace${BIN_SUFFIX}")

add_executable(${LAUNCHER_BINARY}
  WIN32
  ${launcher_SOURCE}
  ${launcher_HEADERS}
)

CreateSourceGroups(${launcher_SOURCE})

target_link_libraries(
  ${LAUNCHER_BINARY}
  ${SDL2_LIBRARY}
  ${OPENGL_LIBRARIES}
  ${OPENAL_LIBRARY}
  ${wxWidgets_LIBRARIES}
  ${PLATFORM_LIBRARIES}
)

set_target_properties(${LAUNCHER_BINARY} PROPERTIES PROJECT_LABEL "Launcher")

# wxWidgets appears to need c++11 for one or more headers
if(NOT WIN32)
  set_target_properties(${LAUNCHER_BINARY} PROPERTIES COMPILE_FLAGS -std=c++11)
endif()

#
# standalone server GUI
#

# Not for FS1 Demo
if(NOT FS1 OR NOT DEMO)
	if(FS1)
	  set(STANDALONE_BINARY fsstandalone)
	else()
	  set(STANDALONE_BINARY fs2standalone)
	endif()

	add_executable(${STANDALONE_BINARY}
	  WIN32
	  ${standalone_SOURCE}
	  ${standalone_HEADERS}
	)

	CreateSourceGroups(${standalone_SOURCE})

	target_link_libraries(
	  ${STANDALONE_BINARY}
	  ${SDL2_LIBRARY}
	  ${wxWidgets_LIBRARIES}
	  ${LIBWEBSOCKETS_LIBRARIES}
	  ${PLATFORM_LIBRARIES}
	)

	set_target_properties(${STANDALONE_BINARY} PROPERTIES PROJECT_LABEL "Standalone")

	# wxWidgets appears to need c++11 for one or more headers
	if(NOT WIN32)
	  set_target_properties(${STANDALONE_BINARY} PROPERTIES COMPILE_FLAGS -std=c++11)
	endif()
endif()

#
# extra rule to make sure we cleanup all exe variants
#

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
  # FS1 demo
  fsdemo${CMAKE_EXECUTABLE_SUFFIX}
  freespacedemo${CMAKE_EXECUTABLE_SUFFIX}
  # FS1
  fs${CMAKE_EXECUTABLE_SUFFIX}
  freespace${CMAKE_EXECUTABLE_SUFFIX}
  # FS2 demo
  fs2demo${CMAKE_EXECUTABLE_SUFFIX}
  freespace2demo${CMAKE_EXECUTABLE_SUFFIX}
  # FS2
  fs2${CMAKE_EXECUTABLE_SUFFIX}
  freespace2${CMAKE_EXECUTABLE_SUFFIX}
  # standalone GUI
  fsstandalone${CMAKE_EXECUTABLE_SUFFIX}
  fs2standalone${CMAKE_EXECUTABLE_SUFFIX}
)

#
# ##############################################################################

# ##############################################################################
#
# toolset: targets for creating/modifying game assets
#

# don't build toolset unless it's a full build
if(NOT DEMO)

	#
	# AC: anim converter
	#

	add_executable(ac
	  EXCLUDE_FROM_ALL
	  ${ac_SOURCE}
	  ${ac_HEADERS}
	)

	CreateSourceGroups(${ac_SOURCE})

	target_link_libraries(
	  ac
	  code
	  ${SDL2_LIBRARY}
	  ${OPENGL_LIBRARIES}
	  ${OPENAL_LIBRARY}
	  ${LIBWEBSOCKETS_LIBRARIES}
	  ${PLATFORM_LIBRARIES}
	)

	#
	# CFILEARCHIVER: to create VP file archives
	#

	add_executable(cfilearchiver
	  EXCLUDE_FROM_ALL
	  ${cfilearchiver_SOURCE}
	)

	CreateSourceGroups(${cfilearchiver_SOURCE})

	#
	# CRYPTSTRING: string encryption (for embedded cheat codes)
	#

	add_executable(cryptstring
	  EXCLUDE_FROM_ALL
	  ${cryptstring_SOURCE}
	)

	CreateSourceGroups(${cryptstring_SOURCE})

	#
	# NEBEDIT: FS1 style nebula editor/creator
	#

	add_executable(nebedit
	  EXCLUDE_FROM_ALL
	  WIN32
	  ${nebedit_SOURCE}
	)

	CreateSourceGroups(${nebedit_SOURCE})

	target_link_libraries(
	  nebedit
	  code
	  ${SDL2_LIBRARY}
	  ${OPENGL_LIBRARIES}
	  ${OPENAL_LIBRARY}
	  ${wxWidgets_LIBRARIES}
	  ${LIBWEBSOCKETS_LIBRARIES}
	  ${PLATFORM_LIBRARIES}
	)

	# wxWidgets appears to need c++11 for one or more headers
	if(NOT WIN32)
	  set_target_properties(nebedit PROPERTIES COMPILE_FLAGS -std=c++11)
	endif()

	#
	# POFVIEW: model viewer
	#

	add_executable(pofview
	  EXCLUDE_FROM_ALL
	  WIN32
	  ${pofview_SOURCE}
	  ${pofview_HEADERS}
	)

	CreateSourceGroups(${pofview_SOURCE})

	target_link_libraries(
	  pofview
	  code
	  ${SDL2_LIBRARY}
	  ${OPENGL_LIBRARIES}
	  ${OPENAL_LIBRARY}
	  ${wxWidgets_LIBRARIES}
	  ${LIBWEBSOCKETS_LIBRARIES}
	  ${PLATFORM_LIBRARIES}
	)

	# wxWidgets appears to need c++11 for one or more headers
	if(NOT WIN32)
	  set_target_properties(pofview PROPERTIES COMPILE_FLAGS -std=c++11)
	endif()

	#
	# SCRAMBLE: file-based encryption for TBLs
	#

	add_executable(scramble
	  EXCLUDE_FROM_ALL
	  ${scramble_SOURCE}
	  ${scramble_HEADERS}
	)

	CreateSourceGroups(${scramble_SOURCE})

	#
	# FONTTOOL: create font files / edit kerning data
	#

	add_executable(fonttool
	  EXCLUDE_FROM_ALL
	  ${fonttool_SOURCE}
	  ${fonttool_HEADERS}
	)

	CreateSourceGroups(${fonttool_SOURCE})

	# background setup, to allow running from build location without installing
	add_custom_command(TARGET fonttool
	  POST_BUILD
	  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/src/fonttool"
	  COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/src/fonttool/fonttool.pcx" "${CMAKE_CURRENT_BINARY_DIR}/src/fonttool"
	)

	target_link_libraries(
	  fonttool
	  code
	  ${SDL2_LIBRARY}
	  ${OPENGL_LIBRARIES}
	  ${OPENAL_LIBRARY}
	  ${LIBWEBSOCKETS_LIBRARIES}
	  ${PLATFORM_LIBRARIES}
	)

	#
	# custom target to build all tools in one pass
	#

	add_custom_target(tools)

	add_dependencies(tools
	  ac
	  cfilearchiver
	  cryptstring
	  nebedit
	  pofview
	  scramble
	  fonttool
	)

endif(NOT DEMO)

#
# ##############################################################################

#
# optionally include any dev/user preferred build commands and/or options
#

include(custom.cmake OPTIONAL)
