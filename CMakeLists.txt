
project(freespace2)

cmake_minimum_required(VERSION 2.6)


include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)
include(CheckTypeSize)
include(FindPkgConfig)

pkg_search_module(SDL2 REQUIRED sdl2 >= 2.0.1)

find_package(OpenGL)
find_package(OpenAL)

option(FS1 "Build original FreeSpace" OFF)
option(DEMO "Create demo build" OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
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

add_subdirectory(src)

include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${SDL2_INCLUDE_DIRS}
  ${OPENAL_INCLUDE_DIR}
)

set(fs_SOURCE
  src/freespace2/freespace.cpp
  src/freespace2/levelpaging.cpp
  src/freespace2/main.cpp
)

include(includes.cmake)

set(FS_BINARY freespace2)

if(FS1)
  add_definitions(-DMAKE_FS1)
  if(DEMO)
    add_definitions(-DDEMO -DFS1_DEMO)
    set(FS_BINARY freespace_demo)
  else()
    set(FS_BINARY freespace)
  endif()
else()
  if(DEMO)
    add_definitions(-DFS2_DEMO)
    set(FS_BINARY freespace2_demo)
  else()
    set(FS_BINARY freespace2)
  endif()
endif()

add_executable(${FS_BINARY} ${fs_SOURCE} ${fs_INCLUDES})

target_link_libraries(
  ${FS_BINARY}
  code
  ${SDL2_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${OPENAL_LIBRARY}
)

add_executable(cfilearchiver
  EXCLUDE_FROM_ALL
  src/cfilearchiver/cfilearchiver.cpp
)

add_executable(scramble
  EXCLUDE_FROM_ALL
  src/platform/unix.cpp
  src/parse/encrypt.cpp
  src/scramble/scramble.cpp
)

add_executable(cryptstring
  EXCLUDE_FROM_ALL
  src/cryptstring/cryptstring.cpp
)

add_executable(fonttool
  EXCLUDE_FROM_ALL
  src/fonttool/fontstubs.cpp
  src/fonttool/fontcreate.cpp
  src/fonttool/fontkern.cpp
  src/fonttool/fontkerncopy.cpp
  src/fonttool/fonttool.cpp
)

target_link_libraries(
  fonttool
  code
  ${SDL2_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${OPENAL_LIBRARY}
)

add_custom_target(tools)

add_dependencies(tools
  cfilearchiver
  scramble
  cryptstring
  fonttool
)

include(custom.cmake OPTIONAL)
