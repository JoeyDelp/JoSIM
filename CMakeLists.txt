# SpecIFy the minimum version for CMake
cmake_minimum_required(VERSION 3.13)

# The version number.
set(JoSIM_VERSION "2.2")

# Project name
project(JoSIM VERSION ${JoSIM_VERSION})

include(${PROJECT_SOURCE_DIR}/cmake/conan.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/make_static_target.cmake)

# Default to release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Ensure C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# See if OpenMP was specified
option(USING_OPENMP "Allow parallel processing in JoSIM" OFF)
option(MAKING_STATIC_BUILD "Build josim to be as static as possible" OFF)

if(MAKING_STATIC_BUILD)
  set(CMAKE_BUILD_SHARED OFF)
endif()

# CMake utils
include(${PROJECT_SOURCE_DIR}/cmake/suitesparse.cmake)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(PLATFORM "win")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(PLATFORM "linux")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(PLATFORM "mac")
else()
  message(SEND_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

# Add JoSIM library
add_library(josim
            src/CliOptions.cpp
            src/j_simulation.cpp
            src/j_output.cpp
            src/j_misc.cpp
            src/j_matrix.cpp
            src/j_input.cpp
            src/j_errors.cpp
            src/j_components.cpp
            src/j_parser.cpp
            src/j_verbose.cpp)

target_include_directories(josim PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(josim PUBLIC suitesparse::klu)
set_target_properties(josim
                      PROPERTIES POSITION_INDEPENDENT_CODE
                                 ON)
make_target_static(josim)

if(USING_OPENMP)
  find_package(OpenMP REQUIRED)
  target_link_libraries(josim PUBLIC OpenMP::OpenMP_CXX)
endif()

# Add JoSIM executable
add_executable(josim-cli src/josim.cpp)
target_link_libraries(josim-cli PRIVATE josim)
make_target_static(josim-cli)

# add compiler warnings
include(${PROJECT_SOURCE_DIR}/cmake/warnings.cmake)
target_add_warnings(josim)
target_add_warnings(josim-cli)

# Create python bindings
add_subdirectory(python)

# set_target_properties( josim-cli PROPERTIES OUTPUT_NAME
# ${PROJECT_NAME}_${PLATFORM}_${CMAKE_BUILD_TYPE}_${PLOTTING_ENGINE})
# set(EXECUTABLE_NAME
# ${PROJECT_NAME}_${PLATFORM}_${CMAKE_BUILD_TYPE}_${PLOTTING_ENGINE})

enable_testing()
include(${PROJECT_SOURCE_DIR}/cmake/integration_test.cmake)
add_subdirectory(test)
