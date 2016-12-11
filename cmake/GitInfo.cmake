
find_package(Git)

if(Git_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  execute_process(
    COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
    WORKING_DIRECTORY ${SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  execute_process(
    COMMAND ${GIT_EXECUTABLE} log -1 --format=%ad --date=format:%Y%m%d
    WORKING_DIRECTORY ${SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_DATE
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe
    WORKING_DIRECTORY ${SOURCE_DIR}
    OUTPUT_VARIABLE GIT_TAG
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if(${GIT_TAG})
    message("Building for Git version: ${GIT_COMMIT_DATE}~${GIT_BRANCH}:${GIT_COMMIT_HASH} (${GIT_TAG})")
  else()
    message("Building for Git version: ${GIT_COMMIT_DATE}~${GIT_BRANCH}:${GIT_COMMIT_HASH}")
  endif()
endif()

configure_file(
  ${SOURCE_DIR}/include/gitinfo.h.in
  ${BINARY_DIR}/include/gitinfo.h
)
