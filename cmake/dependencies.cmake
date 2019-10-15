include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)

CPMAddPackage(
  NAME CMakeSuiteSparse
  GITHUB_REPOSITORY pleroux0/CMakeSuiteSparse
  VERSION 0.1.0
  GIT_TAG 487fbd9f9159efb85e30ec3b7f3fef93b45968f0
)

CPMAddPackage(
  NAME Catch2
  GITHUB_REPOSITORY catchorg/Catch2
  VERSION 2.5.0
)
