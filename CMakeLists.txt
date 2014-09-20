
project(freespace2)

cmake_minimum_required(VERSION 2.6)


include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)
include(CheckTypeSize)
include(FindPkgConfig)

pkg_search_module(SDL2 REQUIRED sdl2 >= 2.0.1)

find_package(OpenGL)
find_package(OpenAL)
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
  add_definitions(-DPLAT_UNIX)
  add_definitions(-Wall -Wno-format-y2k -Wno-deprecated)
  add_definitions(-fsigned-char)

  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DNDEBUG -g -O2")
  set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -DNDEBUG -Os")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O2")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3")
endif()

# ##############################################################################


include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${SDL2_INCLUDE_DIRS}
  ${OPENAL_INCLUDE_DIR}
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
  ${freespace_SOURCE}
  ${freespace_HEADERS}
)

target_link_libraries(
  ${FS_BINARY}
  code
  ${SDL2_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${OPENAL_LIBRARY}
)

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
  ${SDL2_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${OPENAL_LIBRARY}
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
  ${nebedit_SOURCE}
)

target_link_libraries(
  nebedit
  code
  ${SDL2_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${OPENAL_LIBRARY}
  ${wxWidgets_LIBRARIES}
)

#
# POFVIEW: model viewer
#

add_executable(pofview
  EXCLUDE_FROM_ALL
  ${pofview_SOURCE}
  ${pofview_HEADERS}
)

target_link_libraries(
  pofview
  code
  ${SDL2_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${OPENAL_LIBRARY}
  ${wxWidgets_LIBRARIES}
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
  ${SDL2_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${OPENAL_LIBRARY}
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
