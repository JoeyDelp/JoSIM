include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)

CPMAddPackage(
  NAME CMakeSuiteSparse
  GITHUB_REPOSITORY JoeyDelp/CMakeSuiteSparse
  VERSION 0.1.0
  GIT_TAG single_precision
)

CPMAddPackage(
  NAME Catch2
  GITHUB_REPOSITORY catchorg/Catch2
  VERSION 2.5.0
)
