include(FetchContent)

# preferred versions
set(SDL_VERSION "3.2.30")
set(OAL_VERSION "1.24.3")
set(LWS_VERSION "4.4.1")
set(ANGLE_VERSION "angle-ebd9856")


# save current compiler flags and disable warnings (gcc/clang)
if(NOT MSVC)
  set(C_FLAGS_save ${CMAKE_C_FLAGS})
  set(CXX_FLAGS_save ${CMAKE_CXX_FLAGS})
  set(SHARED_LINKER_FLAGS_save ${CMAKE_SHARED_LINKER_FLAGS})

  set(CMAKE_C_FLAGS "-w")
  set(CMAKE_CXX_FLAGS "-w")
  set(CMAKE_SHARED_LINKER_FLAGS "-w")
endif()

set(IS_ARM64 FALSE)

if (NOT "${CMAKE_GENERATOR_PLATFORM}" STREQUAL "")
  if(CMAKE_GENERATOR_PLATFORM MATCHES "^(aarch64|arm64|ARM64)")
    set(IS_ARM64 TRUE)
  endif()
elseif(NOT "$ENV{VSCMD_ARG_TGT_ARCH}" STREQUAL "")
  if($ENV{VSCMD_ARG_TGT_ARCH} MATCHES "^(aarch64|arm64|ARM64)")
    set(IS_ARM64 TRUE)
  endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)")
  set(IS_ARM64 TRUE)
endif()

set(IS_64BIT FALSE)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(IS_64BIT TRUE)
else()
  set(IS_64BIT FALSE)
endif()

#
# SDL3
#

FetchContent_Declare(
  SDL3
  URL https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/SDL3-${SDL_VERSION}.zip
  DOWNLOAD_EXTRACT_TIMESTAMP OFF
  EXCLUDE_FROM_ALL
  SYSTEM
)

FetchContent_MakeAvailable(SDL3)

target_set_folder(SDL3-shared "External")
target_set_folder(SDL3_test "External")
target_set_folder(SDL_uclibc "External")

#
# OpenAL (openal-soft)
#

FetchContent_Declare(
  OpenAL
  URL https://github.com/kcat/openal-soft/archive/refs/tags/${OAL_VERSION}.zip
  DOWNLOAD_EXTRACT_TIMESTAMP OFF
  EXCLUDE_FROM_ALL
  SYSTEM
)

# disable basic build/install stuff for openal-soft
set(ALSOFT_UTILS OFF CACHE BOOL "")
set(ALSOFT_NO_CONFIG_UTIL ON CACHE BOOL "")
set(ALSOFT_EXAMPLES OFF CACHE BOOL "")
set(ALSOFT_TESTS OFF CACHE BOOL "")
set(ALSOFT_INSTALL OFF CACHE BOOL "")
set(ALSOFT_INSTALL_CONFIG OFF CACHE BOOL "")
set(ALSOFT_INSTALL_HRTF_DATA OFF CACHE BOOL "")
set(ALSOFT_INSTALL_AMBDEC_PRESETS OFF CACHE BOOL "")
set(ALSOFT_INSTALL_EXAMPLES OFF CACHE BOOL "")
set(ALSOFT_INSTALL_UTILS OFF CACHE BOOL "")
set(ALSOFT_EAX OFF CACHE BOOL "")

# rtkit code causes issues on Mac for some reason
if(APPLE)
	set(ALSOFT_RTKIT OFF CACHE BOOL "")
endif()

# disable all but default backends (we only use loopback)
set(ALSOFT_BACKEND_PIPEWIRE OFF CACHE BOOL "")
set(ALSOFT_BACKEND_PULSEAUDIO OFF CACHE BOOL "")
set(ALSOFT_BACKEND_ALSA OFF CACHE BOOL "")
set(ALSOFT_BACKEND_OSS OFF CACHE BOOL "")
set(ALSOFT_BACKEND_SOLARIS OFF CACHE BOOL "")
set(ALSOFT_BACKEND_SNDIO OFF CACHE BOOL "")
set(ALSOFT_BACKEND_SNDIO OFF CACHE BOOL "")
set(ALSOFT_BACKEND_WINMM OFF CACHE BOOL "")
set(ALSOFT_BACKEND_DSOUND OFF CACHE BOOL "")
set(ALSOFT_BACKEND_WASAPI OFF CACHE BOOL "")
set(ALSOFT_BACKEND_OTHERIO OFF CACHE BOOL "")
set(ALSOFT_BACKEND_JACK OFF CACHE BOOL "")
set(ALSOFT_BACKEND_COREAUDIO OFF CACHE BOOL "")
set(ALSOFT_BACKEND_OBOE OFF CACHE BOOL "")
set(ALSOFT_BACKEND_OPENSL OFF CACHE BOOL "")
set(ALSOFT_BACKEND_PORTAUDIO OFF CACHE BOOL "")
set(ALSOFT_BACKEND_SDL3 OFF CACHE BOOL "")
set(ALSOFT_BACKEND_SDL2 OFF CACHE BOOL "")
set(ALSOFT_BACKEND_WAVE OFF CACHE BOOL "")

# CoreFoundation required on Mac in order to link openal lib
# (normally added with CoreAudio backend)
if(APPLE)
  find_library(COREFOUNDATION_FRAMEWORK NAMES CoreFoundation)
  if(COREFOUNDATION_FRAMEWORK)
    set(EXTRA_LIBS "-Wl,-framework,CoreFoundation" CACHE STRING "")
  endif()
endif()

FetchContent_MakeAvailable(OpenAL)

target_set_folder(OpenAL "External")
target_set_folder(alsoft.common "External")
target_set_folder(alsoft.excommon "External")
target_set_folder(alsoft.fmt "External")
target_set_folder(clang-tidy-check "External")

if(XCODE)
  set_target_properties(alsoft.common PROPERTIES
    STATIC_LIBRARY_OPTIONS "-no_warning_for_no_symbols"
  )
endif()

if(NOT EMSCRIPTEN)

  #
  # libwebsockets
  #

  FetchContent_Declare(
    LibWebSockets
    URL https://github.com/warmcat/libwebsockets/archive/refs/tags/v${LWS_VERSION}.zip
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    EXCLUDE_FROM_ALL
    SYSTEM
  )

  set(BUILD_TESTING OFF CACHE BOOL "")
  set(LWS_WITH_SSL OFF CACHE BOOL "")
  set(LWS_WITH_MINIMAL_EXAMPLES OFF CACHE BOOL "")
  set(LWS_WITHOUT_CLIENT ON CACHE BOOL "")
  set(LWS_WITHOUT_TESTAPPS ON CACHE BOOL "")

  FetchContent_MakeAvailable(LibWebSockets)

  target_set_folder(websockets "External")
  target_set_folder(websockets_shared "External")
  target_set_folder(GENHDR "External")

  if((APPLE OR WIN32) AND IS_64BIT)
    #
    # ANGLE binaries
    #

    if(APPLE)
      set(ANGLE_BIN "angle-mac-universal")
    elseif(IS_ARM64)
      set(ANGLE_BIN "angle-windows-arm64")
    else()
      set(ANGLE_BIN "angle-windows-x64")
    endif()

    FetchContent_Declare(
      ANGLE
      URL https://github.com/jeremyfa/build-angle/releases/download/${ANGLE_VERSION}/${ANGLE_BIN}.zip
    )

    FetchContent_MakeAvailable(ANGLE)

    add_custom_target(ANGLE_LIBS)

    target_set_folder(ANGLE_LIBS "External")

    if(APPLE)
      set(angle_src "${FETCHCONTENT_BASE_DIR}/angle-src/lib")
    else()
      set(angle_src "${FETCHCONTENT_BASE_DIR}/angle-src/bin")
    endif()

   	file(GLOB angle_lib_files
      RELATIVE "${angle_src}"
      "${angle_src}/*"
    )

    add_custom_command(TARGET ANGLE_LIBS PRE_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
        ${angle_lib_files}
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
      WORKING_DIRECTORY "${angle_src}"
      COMMENT "Copying ANGLE libraries..."
    )
  endif((APPLE OR WIN32) AND IS_64BIT)

endif(NOT EMSCRIPTEN)


# reset compiler flags (gcc/clang)
if(NOT MSVC)
  set(CMAKE_C_FLAGS ${C_FLAGS_save})
  set(CMAKE_CXX_FLAGS ${CXX_FLAGS_save})
  set(CMAKE_SHARED_LINKER_FLAGS ${SHARED_LINKER_FLAGS_save})
endif()
