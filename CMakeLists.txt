
cmake_minimum_required(VERSION 3.22...4.0)

include(cmake/utils.cmake)

# these need to be set before project() is called
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS deployment target")
# (currently broken for OpenAL, it must build each separately)
#set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS build architecture(s)")

project(freespace2)

enable_language(C CXX)

if(APPLE)
  enable_language(OBJC OBJCXX)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

if(WIN32)
  # place dependency dlls in runtime directory on Windows
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
endif()

# set external dependencies first so they don't inherit our build flags
include(cmake/dependencies.cmake)

# we need c++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

include(CreateSourceGroups)

# save out some info for editors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ##############################################################################

if(EMSCRIPTEN)
  set(GAME_ONLY ON CACHE FORCE) # just the game
  set(DEMO ON CACHE FORCE) # only build demo
else()
  option(GAME_ONLY "Just build the game engine; no tools" OFF)
  option(DEMO "Create demo build" OFF)
endif()

option(FS1 "Build original FreeSpace" OFF)
option(FS2_TEX_PACK "Download HQ texture pack for FreeSpace 2" ON)

set(DEMO_GAME_DATA "" CACHE PATH "Path to demo data files used when packaging")

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

if(EMSCRIPTEN)
  add_definitions(-s USE_SDL=2)
  add_definitions(-s FULL_ES2=1)

  set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

if(WIN32)
  set(PLATFORM_LIBRARIES
    ws2_32
    shlwapi
  )

  add_definitions(/D _CRT_SECURE_NO_WARNINGS)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4100") # unreferenced parameter
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4127") # conditional expression is constant: do { } while (0)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4996") # deprecated functions: fopen, fileno, ...
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4505") # unreferenced function with internal linkage

  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MD /D RELEASE_REAL")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /D RELEASE_REAL")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
else()
  set(PLATFORM_LIBRARIES "")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsigned-char")

  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g -Os -DRELEASE_REAL")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os -DRELEASE_REAL")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
endif()

set(BIN_SUFFIX "")

if(NOT FS1)
	set(BIN_SUFFIX "2")
endif()

if(DEMO)
	set(BIN_SUFFIX "${BIN_SUFFIX}demo")
endif()

set(FS_BINARY "freespace${BIN_SUFFIX}")

set(APP_ID "org.icculus.freespace2.${FS_BINARY}")

include(cmake/version.cmake)


include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${CMAKE_BINARY_DIR}/include
)

# ##############################################################################


add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(lib)

add_subdirectory(tools)	# tools needed for building


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

set_target_properties(buildver PROPERTIES
  FOLDER "Build Tools"
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
  ${ext_HEADERS}
)

target_link_libraries(
  code
  embedvp
  SDL3::SDL3
  OpenAL::OpenAL
  ${LIBWEBSOCKETS_LIBRARIES}
)

target_compile_definitions(code
  PUBLIC
  APP_ID="${APP_ID}"
  FS_BINARY="${FS_BINARY}"
)

set_target_properties(code PROPERTIES
  STATIC_LIBRARY_OPTIONS "-no_warning_for_no_symbols"
)

target_precompile_headers(code
   PRIVATE
     include/pstypes.h
)

CreateSourceGroups(${code_SOURCE} ${platform_SOURCE})

add_dependencies(code
  buildver
)

#
# the game itself
#

add_executable(${FS_BINARY}
  WIN32
  ${freespace_SOURCE}
  ${freespace_HEADERS}
  ${launcher_SOURCE}
  ${launcher_HEADERS}
)

target_precompile_headers(${FS_BINARY} REUSE_FROM code)

if(TARGET ANGLE_LIBS)
  add_dependencies(${FS_BINARY}
    ANGLE_LIBS
  )
endif()

CreateSourceGroups(${freespace_SOURCE} ${launcher_SOURCE})

target_link_libraries(
  ${FS_BINARY}
  code
  imgui
  ValveFileVDF
  OpenAL::OpenAL
  SDL3::SDL3
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

  configure_file(${CMAKE_SOURCE_DIR}/dist/emscripten/demo_shell.html.in ${CMAKE_BINARY_DIR}/dist/demo_shell.html @ONLY)
  configure_file(${CMAKE_SOURCE_DIR}/dist/emscripten/demo-play.css.in ${CMAKE_BINARY_DIR}/dist/demo-play.css @ONLY)
  configure_file(${CMAKE_SOURCE_DIR}/dist/emscripten/demo-play.js.in ${CMAKE_BINARY_DIR}/dist/demo-play.js @ONLY)
endif(EMSCRIPTEN)

if(FS1)
  set_target_properties(${FS_BINARY} PROPERTIES PROJECT_LABEL "FreeSpace")
else()
  set_target_properties(${FS_BINARY} PROPERTIES PROJECT_LABEL "FreeSpace2")
endif()

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
  freespace2${CMAKE_EXECUTABLE_SUFFIX}
)

#
# ##############################################################################

# ##############################################################################
#
# toolset: targets for creating/modifying game assets
#

# don't build toolset unless it's a full build
if(NOT GAME_ONLY AND NOT DEMO)

  #
  # AC: anim converter
  #

  add_executable(ac
    EXCLUDE_FROM_ALL
    ${ac_SOURCE}
    ${ac_HEADERS}
  )

  target_precompile_headers(ac REUSE_FROM code)

  CreateSourceGroups(${ac_SOURCE})

  set_target_properties(ac PROPERTIES
    FOLDER "Toolset"
  )

  target_link_libraries(
    ac
    code
    SDL3::SDL3
    OpenAL::OpenAL
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

  set_target_properties(cfileutil PROPERTIES
    FOLDER "Toolset"
  )

  target_link_libraries(
    cfileutil
    SDL3::Headers
  )

  #
  # CRYPTSTRING: string encryption (for embedded cheat codes)
  #

  add_executable(cryptstring
    EXCLUDE_FROM_ALL
    ${cryptstring_SOURCE}
  )

  CreateSourceGroups(${cryptstring_SOURCE})

  set_target_properties(cryptstring PROPERTIES
    FOLDER "Toolset"
  )

  #
  # NEBEDIT: FS1 style nebula editor/creator
  #

  add_executable(nebedit
    EXCLUDE_FROM_ALL
    WIN32
    ${nebedit_SOURCE}
  )

  target_precompile_headers(nebedit REUSE_FROM code)

  CreateSourceGroups(${nebedit_SOURCE})

  set_target_properties(nebedit PROPERTIES
    FOLDER "Toolset"
  )

  target_link_libraries(
    nebedit
    code
    SDL3::SDL3
    OpenAL::OpenAL
    ${LIBWEBSOCKETS_LIBRARIES}
    ${PLATFORM_LIBRARIES}
  )

  #
  # POFVIEW: model viewer
  #

  #add_executable(pofview
  #  EXCLUDE_FROM_ALL
  #  WIN32
  #  ${pofview_SOURCE}
  #  ${pofview_HEADERS}
  #)

  #CreateSourceGroups(${pofview_SOURCE})

  #set_target_properties(pofview PROPERTIES
  #  FOLDER "Toolset"
  #)

  #target_link_libraries(
  #  pofview
  #  code
  #  ${OPENGL_LIBRARIES}
  #  SDL3::SDL3
  #  OpenAL::OpenAL
  #  wx::core
  #  wx::base
  #  wx::gl
  #  ${LIBWEBSOCKETS_LIBRARIES}
  #  ${PLATFORM_LIBRARIES}
  #)

  #
  # SCRAMBLE: file-based encryption for TBLs
  #

  add_executable(scramble
    EXCLUDE_FROM_ALL
    ${scramble_SOURCE}
    ${scramble_HEADERS}
  )

  target_link_libraries(
    scramble
    SDL3::Headers
  )

  CreateSourceGroups(${scramble_SOURCE})

  set_target_properties(scramble PROPERTIES
    FOLDER "Toolset"
  )

  #
  # FONTTOOL: create font files / edit kerning data
  #

  add_executable(fonttool
    EXCLUDE_FROM_ALL
    ${fonttool_SOURCE}
    ${fonttool_HEADERS}
  )

  target_precompile_headers(fonttool REUSE_FROM code)

  CreateSourceGroups(${fonttool_SOURCE})

  set_target_properties(fonttool PROPERTIES
    FOLDER "Toolset"
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
    SDL3::SDL3
    OpenAL::OpenAL
    ${LIBWEBSOCKETS_LIBRARIES}
    ${PLATFORM_LIBRARIES}
  )

  #
  # FRED
  #

  if(WIN32 AND NOT FS1)
    set(FRED_BINARY "fred2")

    add_executable(${FRED_BINARY}
        EXCLUDE_FROM_ALL
        WIN32
        ${fred_SOURCE}
        ${fred_HEADERS}
    )

    CreateSourceGroups(${fred_SOURCE})

    target_include_directories(${FRED_BINARY} BEFORE
        PUBLIC ${PROJECT_SOURCE_DIR}/include/fred2
    )

    set_target_properties(${FRED_BINARY} PROPERTIES PROJECT_LABEL "FRED2")

    # target Windows Vista and newer
    target_compile_definitions(${FRED_BINARY} PUBLIC _WIN32_WINNT=0x0600 _AFXDLL)

    target_link_libraries(
        ${FRED_BINARY}
        code
        imgui
        SDL3::SDL3
        OpenAL::OpenAL
        ${LIBWEBSOCKETS_LIBRARIES}
        ${PLATFORM_LIBRARIES}
        winmm
        opengl32
    )
  endif()

  #
  # custom target to build all tools in one pass
  #

  add_custom_target(toolset)

  set_target_properties(toolset PROPERTIES
    FOLDER "Toolset"
  )

  add_dependencies(toolset
    ac
    cfileutil
    cryptstring
    fonttool
    nebedit
  #  pofview
    scramble
  )

  if(WIN32 AND NOT FS1)
    add_dependencies(toolset
        fred2
    )
  endif()

endif(NOT GAME_ONLY AND NOT DEMO)

#
# ##############################################################################

#
# any installer/packager configurations come last and should be specified as
# separate files included here as necessary
#

if(APPLE)
  include(CreateMacOSBundle)
elseif(LINUX)
  include(CreateAppImage)
elseif(WIN32)
  include(CreateWindowsArchive)
endif()
