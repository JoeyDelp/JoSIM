cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
  set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
endif()

project(JoSIM VERSION 2.7)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT DEFINED CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 11)
endif()
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(SLU "Retired SuperLU selection (ON is unsupported)" OFF)
if(SLU)
  message(FATAL_ERROR "SuperLU backend has been retired. KLU is the supported solver; remove -DSLU=ON.")
endif()
option(USING_OPENMP "Allow parallel processing in JoSIM" OFF)
option(MAKING_STATIC_BUILD "Build josim to be as static as possible" OFF)
if(MAKING_STATIC_BUILD)
  set(BUILD_SHARED_LIBS OFF)
  if(MSVC)
    if(CMAKE_VERSION VERSION_LESS 3.15)
      message(FATAL_ERROR "Static Windows runtimes require CMake 3.15 or newer")
    endif()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  endif()
endif()

include(${PROJECT_SOURCE_DIR}/cmake/verification.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/dependencies.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/make_static_target.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/warnings.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/commit_hash.cmake)

add_definitions(-DVERSION="${CMAKE_PROJECT_VERSION}")
if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()
# Default to release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# JoSIM library
# -------------

add_library(
  josim
  src/Capacitor.cpp
  src/CCCS.cpp
  src/CCVS.cpp
  src/CliOptions.cpp
  src/CurrentSource.cpp
  src/Errors.cpp
  src/Function.cpp
  src/Inductor.cpp
  src/Input.cpp
  src/JJ.cpp
  src/Matrix.cpp
  src/Misc.cpp
  src/Model.cpp
  src/Netlist.cpp
  src/Output.cpp
  src/Parameters.cpp
  src/PhaseSource.cpp
  src/RelevantTrace.cpp
  src/Resistor.cpp
  src/Rng.cpp
  src/Simulation.cpp
  src/Transient.cpp
  src/TransmissionLine.cpp
  src/VCCS.cpp
  src/VCVS.cpp
  src/Verbose.cpp
  src/VoltageSource.cpp
  src/Noise.cpp
  src/Spread.cpp
  src/IV.cpp)

# Alias for projects including JoSIM
add_library(josim::josim ALIAS josim)

# Include directories
target_include_directories(
  josim PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
               $<INSTALL_INTERFACE:include>)

# Dependencies
target_link_libraries(josim PUBLIC suitesparse::klu)
target_compile_features(josim PUBLIC cxx_std_17)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(josim PRIVATE Threads::Threads)
if(NOT MSVC AND NOT APPLE)
  target_link_libraries(josim PRIVATE stdc++fs)
endif()

# Properties
set_target_properties(josim PROPERTIES POSITION_INDEPENDENT_CODE ON)
make_target_static(josim)
target_add_warnings(josim)
josim_instrument_target(josim)
# target_compile_features(josim PUBLIC cxx_std_20)

# OpenMP usage
if(USING_OPENMP)
  find_package(OpenMP REQUIRED)
  target_link_libraries(josim PUBLIC OpenMP::OpenMP_CXX)
endif()

# JoSIM command line interface
# ----------------------------

add_executable(josim-cli src/josim.cpp)

# Dependencies
target_link_libraries(josim-cli PRIVATE josim)
target_link_libraries(josim-cli PRIVATE suitesparse::klu)

# Properties
make_target_static(josim-cli)
target_add_warnings(josim-cli)
josim_instrument_target(josim-cli)

# Testing
# -------

# Neccesary includes
include(CTest)
include(${PROJECT_SOURCE_DIR}/cmake/integration_test.cmake)

# Enable testing
enable_testing()

# Add tests
add_subdirectory(test)

# Install
# -------

# Get install locations
include(GNUInstallDirs)

# Install headers
install(DIRECTORY include/JoSIM/
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/JoSIM/")

# Legacy dependency wrappers add an extra include/ directory during install.
# Provide their headers at the advertised prefix/include as well.
foreach(component config amd btf colamd klu)
  install(DIRECTORY "${CMakeSuiteSparse_SOURCE_DIR}/${component}/include/"
          DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
endforeach()

install(FILES LICENSE README.md CHANGELOG.md DESTINATION .)
install(FILES docs/gate_junctions.md docs/noise_sources.md
              docs/DEVELOPMENT.md docs/RELEASE_POLICY.md DESTINATION docs)

# Install targets
install(
  TARGETS josim josim-cli
  EXPORT josim-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(
  EXPORT josim-targets
  FILE josim-targets.cmake
  NAMESPACE josim::
  DESTINATION share/josim/)

include(CMakePackageConfigHelpers)
configure_package_config_file(cmake/josim-config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/josim-config.cmake"
  INSTALL_DESTINATION share/josim)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/josim-config-version.cmake"
  VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/josim-config.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/josim-config-version.cmake"
        DESTINATION share/josim/)
