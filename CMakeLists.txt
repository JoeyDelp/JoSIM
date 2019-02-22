# SpecIFy the minimum version for CMake
cmake_minimum_required(VERSION 3.13)

# The version number.
set(JoSIM_VERSION "2.2")

# Project name
project(JoSIM VERSION ${JoSIM_VERSION})

# Default to release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Ensure C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# See if OpenMP was specified
option(USING_OPENMP "Allow parallel processing in JoSIM" OFF)

# CMake utils
include(cmake/suitesparse.cmake)

# Set which graphical interface to use USING_[FLTL|MATPLOTLIB|NONE]
option(USING_FLTK "Using FLTK basic graphical interface" OFF)
option(USING_MATPLOTLIB "Using Python based matplotlib graphical interface" OFF)
option(USING_NONE "Using no plotting engine" ON)

include(cmake/plotting.cmake)

if(USING_FLTK)
  enable_fltk_plot()
  set(PLOTTING_ENGINE "FLTK")
endif()

if(USING_MATPLOTLIB)
  enable_matplotlib_plot()
  set(PLOTTING_ENGINE "MATPLOTLIB")
endif()

if(USING_NONE)
  enable_none_plot()
  set(PLOTTING_ENGINE "NONE")
endif()

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
            STATIC
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
target_link_libraries(josim PUBLIC suitesparse::all)
set_target_properties(josim
                      PROPERTIES POSITION_INDEPENDENT_CODE
                                 ON)

if(USING_OPENMP)
  find_package(OpenMP REQUIRED)
  target_link_libraries(josim PUBLIC OpenMP::OpenMP_CXX)
endif()

# Add JoSIM executable
add_executable(josim-cli src/josim.cpp)
target_link_libraries(josim-cli PRIVATE josim)

# add compiler warnings
include(cmake/warnings.cmake)
target_add_warnings(josim)
target_add_warnings(josim-cli)

# Create python bindings
add_subdirectory(python)

# set_target_properties( josim-cli PROPERTIES OUTPUT_NAME
# ${PROJECT_NAME}_${PLATFORM}_${CMAKE_BUILD_TYPE}_${PLOTTING_ENGINE})
# set(EXECUTABLE_NAME
# ${PROJECT_NAME}_${PLATFORM}_${CMAKE_BUILD_TYPE}_${PLOTTING_ENGINE})

enable_testing()
include(cmake/integration_test.cmake)
add_subdirectory(test)
