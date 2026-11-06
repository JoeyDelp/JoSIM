set(JOSIM_SOURCE_REVISION "" CACHE STRING "Source revision supplied by an archive or release build")
if(JOSIM_SOURCE_REVISION)
  string(SUBSTRING "${JOSIM_SOURCE_REVISION}" 0 7 GIT_COMMIT_HASH)
else()
  execute_process(COMMAND git log -1 --format=%h
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_COMMIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET RESULT_VARIABLE git_result)
  if(NOT git_result EQUAL 0)
    set(GIT_COMMIT_HASH "unknown")
  endif()
endif()
add_definitions(-DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}")
