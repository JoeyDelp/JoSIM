cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(JoSIM VERSION 2.4)

include(${PROJECT_SOURCE_DIR}/cmake/dependencies.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/make_static_target.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/warnings.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/commit_hash.cmake)

add_definitions(-DVERSION="${CMAKE_PROJECT_VERSION}")

# Default to release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# See if OpenMP was specified
option(USING_OPENMP "Allow parallel processing in JoSIM" OFF)
option(MAKING_STATIC_BUILD "Build josim to be as static as possible" OFF)

if(MAKING_STATIC_BUILD)
  set(CMAKE_BUILD_SHARED OFF)
endif()

# JoSIM library
# -------------

add_library(josim
            src/CliOptions.cpp
            src/Simulation.cpp
            src/Output.cpp
            src/Misc.cpp
            src/Matrix.cpp
            src/Input.cpp
            src/Errors.cpp
            src/Verbose.cpp
            src/Netlist.cpp
            src/Model.cpp
            src/Parameters.cpp
            src/Transient.cpp
            src/Resistor.cpp
            src/Inductor.cpp
            src/Capacitor.cpp
            src/JJ.cpp
            src/VoltageSource.cpp
            src/PhaseSource.cpp
            src/CurrentSource.cpp
            src/TransmissionLine.cpp
            src/VCCS.cpp
            src/CCCS.cpp
            src/VCVS.cpp
            src/CCVS.cpp
            src/RelevantTrace.cpp)

# Alias for projects including JoSIM
add_library(josim::josim ALIAS josim)

# Include directories
target_include_directories(josim PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

# Dependencies
target_link_libraries(josim PRIVATE suitesparse::klu)

# Properties
set_target_properties(josim
                      PROPERTIES POSITION_INDEPENDENT_CODE
                                 ON)
make_target_static(josim)
target_add_warnings(josim)
target_compile_features(josim PUBLIC cxx_std_17)

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

# Properties
make_target_static(josim-cli)
target_add_warnings(josim-cli)

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
install(DIRECTORY include/JoSIM/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/JoSIM/")

# Install targets
install(TARGETS
    josim
    josim-cli
  EXPORT josim-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

install(EXPORT josim-targets
  FILE josim-config.cmake
  NAMESPACE josim::
  DESTINATION share/josim/
)
