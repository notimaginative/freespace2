
set(FS_VERSION_MAJOR 1)
set(FS_VERSION_MINOR 0)
set(FS_VERSION_BUILD 0)

if(DEFINED BUILD_ID)
  set(FS_VERSION_BUILD ${BUILD_ID})
endif()

if(FS1)
  if(DEMO)
    set(FS_VERSION_MINOR 20)
  else()
    set(FS_VERSION_MINOR 06)
  endif()
else()
  if(DEMO)
    set(FS_VERSION_MINOR 10)
  else()
    set(FS_VERSION_MINOR 20)
  endif()
endif()

set(FS_VERSION "${FS_VERSION_MAJOR}.${FS_VERSION_MINOR}")

if(FS_VERSION_BUILD)
  set(FS_VERSION "${FS_VERSION}.${FS_VERSION_BUILD}")
endif()


configure_file(
  ${CMAKE_SOURCE_DIR}/include/version.h.in
  ${CMAKE_BINARY_DIR}/include/version.h
)
