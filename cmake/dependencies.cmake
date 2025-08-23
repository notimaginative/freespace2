include(FetchContent)

# save current compiler flags and disable warnings
set(C_FLAGS_save ${CMAKE_C_FLAGS})
set(CXX_FLAGS_save ${CMAKE_CXX_FLAGS})
set(SHARED_LINKER_FLAGS_save ${CMAKE_SHARED_LINKER_FLAGS})

if(MSVC)
  set(CMAKE_C_FLAGS "/W0")
  set(CMAKE_CXX_FLAGS "/W0")
else()
  set(CMAKE_C_FLAGS "-w")
  set(CMAKE_CXX_FLAGS "-w")
  set(CMAKE_SHARED_LINKER_FLAGS "-w")
endif()


#
# SDL3
#

FetchContent_Declare(
  SDL3
  #GIT_REPOSITORY https://github.com/libsdl-org/SDL
  #GIT_TAG main
  #GIT_SHALLOW ON
  URL https://github.com/libsdl-org/SDL/releases/download/release-3.2.20/SDL3-3.2.20.zip
  EXCLUDE_FROM_ALL
  SYSTEM
)

FetchContent_MakeAvailable(SDL3)

#
# OpenAL (openal-soft)
#

FetchContent_Declare(
  OpenAL
  #GIT_REPOSITORY https://github.com/kcat/openal-soft
  #GIT_TAG master
  #GIT_SHALLOW ON
  URL https://github.com/kcat/openal-soft/archive/refs/tags/1.24.3.zip
  EXCLUDE_FROM_ALL
  SYSTEM
)

set(ALSOFT_UTILS OFF CACHE BOOL "")
set(ALSOFT_EXAMPLES OFF CACHE BOOL "")
set(ALSOFT_INSTALL OFF CACHE BOOL "")
set(ALSOFT_INSTALL_CONFIG OFF CACHE BOOL "")
set(ALSOFT_INSTALL_HRTF_DATA OFF CACHE BOOL "")
set(ALSOFT_INSTALL_AMBDEC_PRESETS OFF CACHE BOOL "")
set(ALSOFT_INSTALL_EXAMPLES OFF CACHE BOOL "")
set(ALSOFT_INSTALL_UTILS OFF CACHE BOOL "")

if(APPLE)
	set(ALSOFT_RTKIT OFF CACHE BOOL "")
endif()

FetchContent_MakeAvailable(OpenAL)

if(NOT EMSCRIPTEN)
  #
  # libwebsockets
  #

  FetchContent_Declare(
    libwebsockets
    #GIT_REPOSITORY https://github.com/warmcat/libwebsockets
    #GIT_TAG main
    #GIT_SHALLOW ON
    URL https://github.com/warmcat/libwebsockets/archive/refs/tags/v4.3.5.zip
    EXCLUDE_FROM_ALL
    SYSTEM
  )

  set(LWS_WITH_SSL OFF CACHE BOOL "")
  set(LWS_WITHOUT_TESTAPPS ON CACHE BOOL "")

  FetchContent_MakeAvailable(LibWebSockets)
endif()


# reset compiler flags
set(CMAKE_C_FLAGS ${C_FLAGS_save})
set(CMAKE_CXX_FLAGS ${CXX_FLAGS_save})
set(CMAKE_SHARED_LINKER_FLAGS ${SHARED_LINKER_FLAGS_save})
