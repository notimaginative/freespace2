
cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

project(freespace2)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

include(CreateSourceGroups)


# ##############################################################################

option(FS1 "Build original FreeSpace" OFF)

if(EMSCRIPTEN)
  # only build for demo with emscripten
  set(DEMO 1)
else()
  option(DEMO "Create demo build" OFF)
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "RelWithDebInfo" "Release" "Debug")
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

# build everything by default
set(GAME_ONLY)

if(DEMO)
  set(GAME_ONLY 1)
endif()

if(EMSCRIPTEN)
  set(GAME_ONLY 1)  # don't build launcher or toolset
  add_definitions(-s USE_SDL=2)
  add_definitions(-s FULL_ES2=1)

  set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

if(NOT EMSCRIPTEN)
  add_definitions(-DLEGACY_GL)
endif()

if(NOT WIN32)
  set(PLATFORM_LIBRARIES "")

  add_definitions(-DPLAT_UNIX)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsigned-char")

  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g -Os -DRELEASE_REAL")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os -DRELEASE_REAL")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -Og")
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

  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT /D RELEASE_REAL")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /D RELEASE_REAL")
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

if(NOT EMSCRIPTEN)
  find_package(LibWebSockets REQUIRED)

  find_package(wxWidgets COMPONENTS core base gl net)
  include(${wxWidgets_USE_FILE})
endif()

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

if(EMSCRIPTEN)
  set(ASYNC_FUNCTIONS
	# common
    main
    game_main
    game_init
    # fs2 title screen
    display_title_screen
    # loading screen
    game_loop_caller
    gameseq_process_events
    game_process_event
    gameseq_set_state
    game_enter_state
    game_start_mission
    freespace_mission_load_stuff
    game_busy_callback
    game_loading_callback
    game_loading_callback_close
    level_page_in
    bm_page_in_stop
    gamesnd_preload_common_sounds
    game_busy
  )

  list(GET ASYNC_FUNCTIONS -1 LastOne)

  set(EMTERPRETIFY_WHITELIST "[")

  foreach(func ${ASYNC_FUNCTIONS})
    if (${func} STREQUAL ${LastOne})
      set(EMTERPRETIFY_WHITELIST "${EMTERPRETIFY_WHITELIST}\"_${func}\"]")
    else()
      set(EMTERPRETIFY_WHITELIST "${EMTERPRETIFY_WHITELIST}\"_${func}\",")
    endif()
  endforeach()

  set(LINK_FLAGS
    "-O3"
    "-s USE_SDL=2"
    "-s FULL_ES2=1"
    "-s WASM=1"
    "--shell-file ${CMAKE_BINARY_DIR}/dist/demo_shell.html"
    "--pre-js ${FS_BINARY}_preload.js"
    "--emit-symbol-map"
  # "-s EMTERPRETIFY=1"
  # "-s EMTERPRETIFY_ASYNC=1"
  # "-s 'EMTERPRETIFY_FILE=\"${FS_BINARY}.binary\"'"
  # "-s 'EMTERPRETIFY_WHITELIST=${EMTERPRETIFY_WHITELIST}'"
  )

  # memory is calculated as such: 1024 * 1024 * (48 + (data size + 4))
  # 48 to run the game itself, plus in-memory filesystem+data
  # FS1 demo data size is 38MB, FS2 demo is 94MB
  if(FS1)
    set(LINK_FLAGS "${LINK_FLAGS} -s TOTAL_MEMORY=94371840")
  else()
    set(LINK_FLAGS "${LINK_FLAGS} -s TOTAL_MEMORY=153092096")
  endif()

  foreach(Flag ${LINK_FLAGS})
    set_property(TARGET ${FS_BINARY} APPEND_STRING PROPERTY LINK_FLAGS " ${Flag}")
  endforeach()

  if(FS1)
    set(TEXT_COLOR "#4C442D")
    set(BG_COLOR "#3E4559")
    set(OUTLINE_COLOR "#353B4C")
    set(BG_COLOR2 "#798199")
    set(GAME_TITLE "FreeSpace Demo")
  else()
    set(TEXT_COLOR "#FFF")
    set(BG_COLOR "#457573")
    set(OUTLINE_COLOR "#4D8280")
    set(BG_COLOR2 "#182928")
    set(GAME_TITLE "FreeSpace 2 Demo")
  endif()

  configure_file(${CMAKE_SOURCE_DIR}/dist/demo_shell.html.in ${CMAKE_BINARY_DIR}/dist/demo_shell.html @ONLY)
  configure_file(${CMAKE_SOURCE_DIR}/dist/demo-play.css.in ${CMAKE_BINARY_DIR}/dist/demo-play.css @ONLY)
  configure_file(${CMAKE_SOURCE_DIR}/dist/demo-play.js.in ${CMAKE_BINARY_DIR}/dist/demo-play.js @ONLY)
endif(EMSCRIPTEN)

if(FS1)
  set_target_properties(${FS_BINARY} PROPERTIES PROJECT_LABEL "Freespace")
else()
  set_target_properties(${FS_BINARY} PROPERTIES PROJECT_LABEL "Freespace2")
endif()

#
# launcher
#

if(NOT GAME_ONLY)
  set(LAUNCHER_BINARY "freespace${BIN_SUFFIX}")

  add_executable(${LAUNCHER_BINARY}
  WIN32
    ${launcher_SOURCE}
    ${launcher_HEADERS}
  )

  add_dependencies(${LAUNCHER_BINARY}
    buildver
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
endif(NOT GAME_ONLY)

#
# standalone server GUI
#

if(NOT GAME_ONLY)
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
  endif(NOT FS1 OR NOT DEMO)
endif(NOT GAME_ONLY)

#
# extra rule to make sure we cleanup all exe variants
#

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
  # FS1 demo
  fsdemo${CMAKE_EXECUTABLE_SUFFIX}
  fsdemo.html.mem
  fsdemo.binary
  fsdemo.wast
  fsdemo.wasm
  fsdemo.js
  fsdemo.data
  fsdemo_preload.js
  freespacedemo${CMAKE_EXECUTABLE_SUFFIX}
  # FS1
  fs${CMAKE_EXECUTABLE_SUFFIX}
  freespace${CMAKE_EXECUTABLE_SUFFIX}
  # FS2 demo
  fs2demo${CMAKE_EXECUTABLE_SUFFIX}
  fs2demo.html.mem
  fs2demo.binary
  fs2demo.wast
  fs2demo.wasm
  fs2demo.js
  fs2demo.data
  fs2demo_preload.js
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
if(NOT GAME_ONLY)

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
  # CFILEUTIL: work with VP file archives
  #

  add_executable(cfileutil
    EXCLUDE_FROM_ALL
    ${cfileutil_SOURCE}
  )

  CreateSourceGroups(${cfileutil_SOURCE})

  if(NOT WIN32)
    set_target_properties(cfileutil PROPERTIES COMPILE_FLAGS -std=c++11)
  endif()

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
    cfileutil
    cryptstring
    nebedit
    pofview
    scramble
    fonttool
  )

endif(NOT GAME_ONLY)

#
# ##############################################################################

#
# optionally include any dev/user preferred build commands and/or options
#

include(custom.cmake OPTIONAL)
