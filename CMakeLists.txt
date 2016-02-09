
project(freespace2)

cmake_minimum_required(VERSION 2.8)
cmake_policy(VERSION 2.8)

if(POLICY CMP0043)
  cmake_policy(SET CMP0043 OLD)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")


find_package(SDL2 "2.0.1" REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenAL REQUIRED)
find_package(LibWebSockets REQUIRED)

find_package(wxWidgets COMPONENTS core base gl)
include(${wxWidgets_USE_FILE})


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
# add_definitions(-Wno-format-y2k)
# add_definitions(-Wno-deprecated)
  add_definitions(-fsigned-char)

  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DNDEBUG -g -O2")
  set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -DNDEBUG -Os")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O2")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3")
else()
  set(PLATFORM_LIBRARIES
	wsock32.lib
	winmm.lib
  )

  add_definitions(/D _CRT_SECURE_NO_WARNINGS)
  add_definitions(/W4)
  add_definitions(/wd4100) # unreferenced parameter
  add_definitions(/wd4127) # conditional expression is constant: do { } while (0)
  add_definitions(/wd4996) # deprecated functions: fopen, fileno, ...
endif()


# ##############################################################################


include_directories(
  ${PROJECT_SOURCE_DIR}/include
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
# main code/game library
#

add_library(code
  STATIC
  ${code_SOURCE}
  ${code_HEADERS}
  ${platform_SOURCE}
  ${platform_HEADERS}
)

#
# the game itself
#

if(FS1)
  if(DEMO)
    set(FS_BINARY freespace_demo)
  else()
    set(FS_BINARY freespace)
  endif()
else()
  if(DEMO)
    set(FS_BINARY freespace2_demo)
  else()
	set(FS_BINARY freespace2)
  endif()
endif()

add_executable(${FS_BINARY}
  WIN32
  ${freespace_SOURCE}
  ${freespace_HEADERS}
)

target_link_libraries(
  ${FS_BINARY}
  code
  ${SDL2_LIBRARY}
  ${OPENGL_LIBRARIES}
  ${OPENAL_LIBRARY}
  ${LIBWEBSOCKETS_LIBRARIES}
  ${PLATFORM_LIBRARIES}
)

#
# launcher
#

if(FS1)
  set(LAUNCHER_BINARY fslaunch)
else()
  set(LAUNCHER_BINARY fs2launch)
endif()

add_executable(${LAUNCHER_BINARY}
  WIN32
  ${launcher_SOURCE}
  ${launcher_HEADERS}
)

target_link_libraries(
  ${LAUNCHER_BINARY}
  ${SDL2_LIBRARY}
  ${OPENGL_LIBRARIES}
  ${OPENAL_LIBRARY}
  ${wxWidgets_LIBRARIES}
  ${PLATFORM_LIBRARIES}
)

# wxWidgets appears to need c++11 for one or more headers
if(NOT WIN32)
  set_target_properties(${LAUNCHER_BINARY} PROPERTIES COMPILE_FLAGS -std=c++11)
endif()

#
# ##############################################################################

# ##############################################################################
#
# toolset: targets for creating/modifying game assets
#

#
# AC: anim converter
#

add_executable(ac
  EXCLUDE_FROM_ALL
  ${ac_SOURCE}
  ${ac_HEADERS}
)

target_link_libraries(
  ac
  code
  ${SDL2_LIBRARY}
  ${OPENGL_LIBRARIES}
  ${OPENAL_LIBRARY}
  ${PLATFORM_LIBRARIES}
)

#
# CFILEARCHIVER: to create VP file archives
#

add_executable(cfilearchiver
  EXCLUDE_FROM_ALL
  ${cfilearchiver_SOURCE}
)

#
# CRYPTSTRING: string encryption (for embedded cheat codes)
#

add_executable(cryptstring
  EXCLUDE_FROM_ALL
  ${cryptstring_SOURCE}
)

#
# NEBEDIT: FS1 style nebula editor/creator
#

add_executable(nebedit
  EXCLUDE_FROM_ALL
  WIN32
  ${nebedit_SOURCE}
)

target_link_libraries(
  nebedit
  code
  ${SDL2_LIBRARY}
  ${OPENGL_LIBRARIES}
  ${OPENAL_LIBRARY}
  ${wxWidgets_LIBRARIES}
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

target_link_libraries(
  pofview
  code
  ${SDL2_LIBRARY}
  ${OPENGL_LIBRARIES}
  ${OPENAL_LIBRARY}
  ${wxWidgets_LIBRARIES}
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

#
# FONTTOOL: create font files / edit kerning data
#

add_executable(fonttool
  EXCLUDE_FROM_ALL
  WIN32
  ${fonttool_SOURCE}
  ${fonttool_HEADERS}
)

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

#
# ##############################################################################

#
# optionally include any dev/user preferred build commands and/or options
#

include(custom.cmake OPTIONAL)
