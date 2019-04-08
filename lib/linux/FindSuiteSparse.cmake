#.rst:
# FindSuiteSparse
# --------
#
# Find the SuiteSparse library
#
# Input Variables
# ^^^^^^^^^^^^^^^
#
# By default this module will search for all of the SuiteSparse components and
# add them to the SUITESPARSE_LIBRARIES variable.  
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# The following variables will be defined:
#
# ``SUITESPARSE_FOUND``
#   True if all components were found
#
# ``SUITESPARSE_INCLUDE_DIRS``
#   Path to the include directory for SuiteSparse header files
#
# ``SUITESPARSE_LIBRARIES``
#   List of the SuiteSparse libraries found
#
# ``SUITESPARSE_VERSION``
#   Extracted from SuiteSparse_config.h
#
# ``SUITESPARSE_MAIN_VERSION``
#   Main version number
#
# ``SUITESPARSE_SUB_VERSION``
#   Sub version number
#
# ``SUITESPARSE_SUB_SUB_VERSION``
#   Sub sub version number
#
# == Symmetric Approximate Minimum Degree (AMD)
#   AMD_FOUND
#   AMD_INCLUDE_DIR
#   AMD_LIBRARY
#
# == Block Triangulation Form (BTF)
#   BTF_FOUND
#   BTF_INCLUDE_DIR
#   BTF_LIBRARY
#
# == Constrained Approximate Minimum Degree (CAMD)
#   CAMD_FOUND
#   CAMD_INCLUDE_DIR
#   CAMD_LIBRARY
#
# Constrained Column Approximate Minimum Degree (CCOLAMD)
# CCOLAMD_FOUND
# CCOLAMD_INCLUDE_DIR
# CCOLAMD_LIBRARY
#
# == Sparse Supernodal Cholesky Factorization and Update/Downdate (CHOLMOD)
#   CHOLMOD_FOUND
#   CHOLMOD_INCLUDE_DIR
#   CHOLMOD_LIBRARY
#
# == Column Approximate Minimum Degree (COLAMD)
#   COLAMD_FOUND
#   COLAMD_INCLUDE_DIR
#   COLAMD_LIBRARY
#
# == Concise Complex Sparse Matrix Package (CXPARSE)
#   CXPARSE_FOUND
#   CXPARSE_INCLUDE_DIR
#   CXPARSE_LIBRARY
#
# == Sparse LU Factorization (KLU)
#   KLU_FOUND
#   KLU_INCLUDE_DIR
#   KLU_LIBRARY
#
# == Very Concise LDL' Factorization (LDL)
#   LDL_FOUND
#   LDL_INCLUDE_DIR
#   LDL_LIBRARY
#
# == Multifrontal Sparse QR (SuiteSparseQR)
#   SUITESPARSEQR_FOUND
#   SUITESPARSEQR_INCLUDE_DIR
#   SUITESPARSEQR_LIBRARY
#
# == Common configuration.
#   SUITESPARSE_CONFIG_FOUND
#   SUITESPARSE_CONFIG_INCLUDE_DIR
#   SUITESPARSE_CONFIG_LIBRARY
#
# == Sparse LU Factorization (UMFPACK)
#   UMFPACK_FOUND
#   UMFPACK_INCLUDE_DIR
#   UMFPACK_LIBRARY
#
# Optional SuiteSparse Dependencies:
#
# == Serial Graph Partitioning and Fill-reducing Matrix Ordering (METIS)
#   METIS_FOUND
#   METIS_LIBRARY
#
# == Intel Thread Building Blocks (TBB)
#   TBB_FOUND
#   TBB_LIBRARY
#   TBB_MALLOC_FOUND
#   TBB_MALLOC_LIBRARY

# Prefer static over shared libraries
if (MAKING_STATIC_BUILD)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX} ${CMAKE_FIND_LIBRARY_SUFFIXES})
endif()

macro(SUITESPARSE_REPORT_NOT_FOUND REASON_MSG)
  unset(SUITESPARSE_FOUND)
  unset(SUITESPARSE_INCLUDE_DIRS)
  unset(SUITESPARSE_LIBRARIES)
  unset(SUITESPARSE_VERSION)
  unset(SUITESPARSE_MAIN_VERSION)
  unset(SUITESPARSE_SUB_VERSION)
  unset(SUITESPARSE_SUBSUB_VERSION)

  if (SuiteSparse_FIND_QUIETLY)
    message(STATUS "Failed to find SuiteSparse - " ${REASON_MSG} ${ARGN})
  elseif (SuiteSparse_FIND_REQUIRED)
    message(FATAL_ERROR "Failed to find SuiteSparse - " ${REASON_MSG} ${ARGN})
  else()
    message("-- Failed to find SuiteSparse - " ${REASON_MSG} ${ARGN})
  endif (SuiteSparse_FIND_QUIETLY)
endmacro(SUITESPARSE_REPORT_NOT_FOUND)

unset(SUITESPARSE_FOUND)

list(APPEND SUITESPARSE_CHECK_INCLUDE_DIRS
  /opt/local/include
  /usr/local/homebrew/include # Mac OS X
  /usr/local/include
  /usr/include)
list(APPEND SUITESPARSE_CHECK_LIBRARY_DIRS
  /opt/local/lib
  /usr/local/homebrew/lib # Mac OS X
  /usr/local/lib
  /usr/lib)
list(APPEND SUITESPARSE_CHECK_PATH_SUFFIXES
  suitesparse)

macro(suitesparse_find_component COMPONENT)
  include(CMakeParseArguments)
  set(OPTIONS REQUIRED)
  set(MULTI_VALUE_ARGS FILES LIBRARIES)
  cmake_parse_arguments(SUITESPARSE_FIND_${COMPONENT}
    "${OPTIONS}" "" "${MULTI_VALUE_ARGS}" ${ARGN})

  if (SUITESPARSE_FIND_${COMPONENT}_REQUIRED)
    list(APPEND SUITESPARSE_FOUND_REQUIRED_VARS ${COMPONENT}_FOUND)
  endif()

  set(${COMPONENT}_FOUND TRUE)
  if (SUITESPARSE_FIND_${COMPONENT}_FILES)
    find_path(${COMPONENT}_INCLUDE_DIR
      NAMES ${SUITESPARSE_FIND_${COMPONENT}_FILES}
      PATHS ${SUITESPARSE_CHECK_INCLUDE_DIRS}
      PATH_SUFFIXES ${SUITESPARSE_CHECK_PATH_SUFFIXES})
    if (${COMPONENT}_INCLUDE_DIR)
      message(STATUS "Found ${COMPONENT} headers in: "
        "${${COMPONENT}_INCLUDE_DIR}")
      mark_as_advanced(${COMPONENT}_INCLUDE_DIR)
    else()
      # Specified headers not found.
      set(${COMPONENT}_FOUND FALSE)
      if (SUITESPARSE_FIND_${COMPONENT}_REQUIRED)
        suitesparse_report_not_found(
          "Did not find ${COMPONENT} header (required SuiteSparse component).")
      else()
        message(STATUS "Did not find ${COMPONENT} header (optional "
          "SuiteSparse component).")
        # Hide optional vars from CMake GUI even if not found.
        mark_as_advanced(${COMPONENT}_INCLUDE_DIR)
      endif()
    endif()
  endif()

  if (SUITESPARSE_FIND_${COMPONENT}_LIBRARIES)
    find_library(${COMPONENT}_LIBRARY
      NAMES ${SUITESPARSE_FIND_${COMPONENT}_LIBRARIES}
      PATHS ${SUITESPARSE_CHECK_LIBRARY_DIRS}
      PATH_SUFFIXES ${SUITESPARSE_CHECK_PATH_SUFFIXES})
    if (${COMPONENT}_LIBRARY)
      message(STATUS "Found ${COMPONENT} library: ${${COMPONENT}_LIBRARY}")
      mark_as_advanced(${COMPONENT}_LIBRARY)
    else ()
      # Specified libraries not found.
      set(${COMPONENT}_FOUND FALSE)
      if (SUITESPARSE_FIND_${COMPONENT}_REQUIRED)
        suitesparse_report_not_found(
          "Did not find ${COMPONENT} library (required SuiteSparse component).")
      else()
        message(STATUS "Did not find ${COMPONENT} library (optional SuiteSparse "
          "dependency)")
        # Hide optional vars from CMake GUI even if not found.
        mark_as_advanced(${COMPONENT}_LIBRARY)
      endif()
    endif()
  endif()
endmacro()

unset(SUITESPARSE_FOUND_REQUIRED_VARS)

suitesparse_find_component(AMD REQUIRED FILES amd.h LIBRARIES amd)
suitesparse_find_component(BTF REQUIRED FILES btf.h LIBRARIES btf)
suitesparse_find_component(CAMD REQUIRED FILES camd.h LIBRARIES camd)
suitesparse_find_component(CCOLAMD REQUIRED FILES ccolamd.h LIBRARIES ccolamd)
suitesparse_find_component(CHOLMOD REQUIRED FILES cholmod.h LIBRARIES cholmod)
suitesparse_find_component(COLAMD REQUIRED FILES colamd.h LIBRARIES colamd)
suitesparse_find_component(CXPARSE REQUIRED FILES cs.h LIBRARIES cxsparse)
suitesparse_find_component(KLU REQUIRED FILES klu.h LIBRARIES klu)
suitesparse_find_component(LDL REQUIRED FILES ldl.h LIBRARIES ldl)
suitesparse_find_component(SUITESPARSEQR REQUIRED FILES SuiteSparseQR.hpp LIBRARIES spqr)
if (SUITESPARSEQR_FOUND)
  # SuiteSparseQR may be compiled with Intel Threading Building Blocks,
  # we assume that if TBB is installed, SuiteSparseQR was compiled with
  # support for it, this will do no harm if it wasn't.
  find_package(TBB QUIET)
  if (TBB_FOUND)
    message(STATUS "Found Intel Thread Building Blocks (TBB) library "
      "(${TBB_VERSION}) assuming SuiteSparseQR was compiled "
      "with TBB.")
    # Add the TBB libraries to the SuiteSparseQR libraries (the only
    # libraries to optionally depend on TBB).
    list(APPEND SUITESPARSEQR_LIBRARY ${TBB_LIBRARIES})
  else()
    message(STATUS "Did not find Intel TBB library, assuming SuiteSparseQR was "
      "not compiled with TBB.")
  endif()
endif(SUITESPARSEQR_FOUND)

suitesparse_find_component(SUITESPARSE_CONFIG FILES SuiteSparse_config.h LIBRARIES suitesparseconfig)

if (SUITESPARSE_CONFIG_FOUND)
  if (CMAKE_SYSTEM_NAME MATCHES "Linux" OR UNIX AND NOT APPLE)
  suitesparse_find_component(LIBRT LIBRARIES rt)
    if (LIBRT_FOUND)
      message(STATUS "Adding librt: ${LIBRT_LIBRARY} to "
        "SuiteSparse_config libraries (required on Linux & Unix [not OSX] if "
        "SuiteSparse is compiled with timing).")
      list(APPEND SUITESPARSE_CONFIG_LIBRARY ${LIBRT_LIBRARY})
    else()
      message(STATUS "Could not find librt, but found SuiteSparse_config, "
        "assuming that SuiteSparse was compiled without timing.")
    endif ()
  endif (CMAKE_SYSTEM_NAME MATCHES "Linux" OR UNIX AND NOT APPLE)
endif ()

if (NOT SUITESPARSE_CONFIG_FOUND)
  suitesparse_report_not_found(
    "Failed to find: SuiteSparse_config header & library (should be "
    "present in all SuiteSparse >= v4 installs).")
endif()

list(APPEND SUITESPARSE_FOUND_REQUIRED_VARS SUITESPARSE_VERSION)

if (SUITESPARSE_CONFIG_FOUND)
  # SuiteSparse version >= 4.
  set(SUITESPARSE_VERSION_FILE ${SUITESPARSE_CONFIG_INCLUDE_DIR}/SuiteSparse_config.h)
  if (NOT EXISTS ${SUITESPARSE_VERSION_FILE})
    suitesparse_report_not_found(
      "Could not find file: ${SUITESPARSE_VERSION_FILE} containing version "
      "information for >= v4 SuiteSparse installs, but SuiteSparse_config was "
      "found (only present in >= v4 installs).")
  else (NOT EXISTS ${SUITESPARSE_VERSION_FILE})
    file(READ ${SUITESPARSE_VERSION_FILE} SUITESPARSE_CONFIG_CONTENTS)

    string(REGEX MATCH "#define SUITESPARSE_MAIN_VERSION [0-9]+"
      SUITESPARSE_MAIN_VERSION "${SUITESPARSE_CONFIG_CONTENTS}")
    string(REGEX REPLACE "#define SUITESPARSE_MAIN_VERSION ([0-9]+)" "\\1"
      SUITESPARSE_MAIN_VERSION "${SUITESPARSE_MAIN_VERSION}")

    string(REGEX MATCH "#define SUITESPARSE_SUB_VERSION [0-9]+"
      SUITESPARSE_SUB_VERSION "${SUITESPARSE_CONFIG_CONTENTS}")
    string(REGEX REPLACE "#define SUITESPARSE_SUB_VERSION ([0-9]+)" "\\1"
      SUITESPARSE_SUB_VERSION "${SUITESPARSE_SUB_VERSION}")

    string(REGEX MATCH "#define SUITESPARSE_SUBSUB_VERSION [0-9]+"
      SUITESPARSE_SUBSUB_VERSION "${SUITESPARSE_CONFIG_CONTENTS}")
    string(REGEX REPLACE "#define SUITESPARSE_SUBSUB_VERSION ([0-9]+)" "\\1"
      SUITESPARSE_SUBSUB_VERSION "${SUITESPARSE_SUBSUB_VERSION}")

    set(SUITESPARSE_VERSION
      "${SUITESPARSE_MAIN_VERSION}.${SUITESPARSE_SUB_VERSION}.${SUITESPARSE_SUBSUB_VERSION}")
  endif (NOT EXISTS ${SUITESPARSE_VERSION_FILE})
endif (SUITESPARSE_CONFIG_FOUND)

suitesparse_find_component(UMFPACK REQUIRED FILES umfpack.h LIBRARIES umfpack)
suitesparse_find_component(METIS LIBRARIES metis)

set(SUITESPARSE_FOUND TRUE)
foreach(REQUIRED_VAR ${SUITESPARSE_FOUND_REQUIRED_VARS})
  if (NOT ${REQUIRED_VAR})
    set(SUITESPARSE_FOUND FALSE)
  endif (NOT ${REQUIRED_VAR})
endforeach(REQUIRED_VAR ${SUITESPARSE_FOUND_REQUIRED_VARS})

if (SUITESPARSE_FOUND)
  list(APPEND SUITESPARSE_INCLUDE_DIRS
    ${AMD_INCLUDE_DIR}
    ${BTF_INCLUDE_DIR}
    ${CAMD_INCLUDE_DIR}
    ${COLAMD_INCLUDE_DIR}
    ${CCOLAMD_INCLUDE_DIR}
    ${CHOLMOD_INCLUDE_DIR}
    ${CXPARSE_INCLUDE_DIR}
    ${KLU_INCLUDE_DIR}
    ${LDL_INCLUDE_DIR}
    ${SUITESPARSEQR_INCLUDE_DIR}
    ${UMFPACK_INCLUDE_DIR})
  # Handle config separately, as otherwise at least one of them will be set
  # to NOTFOUND which would cause any check on SUITESPARSE_INCLUDE_DIRS to fail.
  if (SUITESPARSE_CONFIG_FOUND)
    list(APPEND SUITESPARSE_INCLUDE_DIRS
      ${SUITESPARSE_CONFIG_INCLUDE_DIR})
  endif (SUITESPARSE_CONFIG_FOUND)
  list(REMOVE_DUPLICATES SUITESPARSE_INCLUDE_DIRS)
  list(APPEND SUITESPARSE_LIBRARIES
    ${SUITESPARSEQR_LIBRARY}
    ${CHOLMOD_LIBRARY}
    ${CCOLAMD_LIBRARY}
    ${CAMD_LIBRARY}
    ${COLAMD_LIBRARY}
    ${AMD_LIBRARY}
    ${CXPARSE_LIBRARY}
    ${KLU_LIBRARY}
    ${LDL_LIBRARY}
    ${UMFPACK_LIBRARY}
    ${BTF_LIBRARY})
  if (SUITESPARSE_CONFIG_FOUND)
    list(APPEND SUITESPARSE_LIBRARIES
      ${SUITESPARSE_CONFIG_LIBRARY})
  endif (SUITESPARSE_CONFIG_FOUND)
  if (METIS_FOUND)
    list(APPEND SUITESPARSE_LIBRARIES
      ${METIS_LIBRARY})
  endif (METIS_FOUND)
endif()

set(SUITESPARSE_IS_BROKEN_SHARED_LINKING_UBUNTU_SYSTEM_VERSION FALSE)
if (CMAKE_SYSTEM_NAME MATCHES "Linux" AND
    SUITESPARSE_VERSION VERSION_EQUAL 3.4.0)
  find_program(LSB_RELEASE_EXECUTABLE lsb_release)
  if (LSB_RELEASE_EXECUTABLE)
    # Any even moderately recent Ubuntu release (likely to be affected by
    # this bug) should have lsb_release, if it isn't present we are likely
    # on a different Linux distribution (should be fine).
    execute_process(COMMAND ${LSB_RELEASE_EXECUTABLE} -si
      OUTPUT_VARIABLE LSB_DISTRIBUTOR_ID
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    if (LSB_DISTRIBUTOR_ID MATCHES "Ubuntu" AND
        SUITESPARSE_LIBRARIES MATCHES "/usr/lib/libamd")
      # We are on Ubuntu, and the SuiteSparse version matches the broken
      # system install version and is a system install.
      set(SUITESPARSE_IS_BROKEN_SHARED_LINKING_UBUNTU_SYSTEM_VERSION TRUE)
      message(STATUS "Found system install of SuiteSparse "
        "${SUITESPARSE_VERSION} running on Ubuntu, which has a known bug "
        "preventing linking of shared libraries (static linking unaffected).")
    endif (LSB_DISTRIBUTOR_ID MATCHES "Ubuntu" AND
      SUITESPARSE_LIBRARIES MATCHES "/usr/lib/libamd")
  endif (LSB_RELEASE_EXECUTABLE)
endif (CMAKE_SYSTEM_NAME MATCHES "Linux" AND
  SUITESPARSE_VERSION VERSION_EQUAL 3.4.0)

# Handle REQUIRED and QUIET arguments to FIND_PACKAGE
include(FindPackageHandleStandardArgs)
if (SUITESPARSE_FOUND)
  find_package_handle_standard_args(SuiteSparse
    REQUIRED_VARS ${SUITESPARSE_FOUND_REQUIRED_VARS}
    VERSION_VAR SUITESPARSE_VERSION
    FAIL_MESSAGE "Failed to find some/all required components of SuiteSparse.")
else (SUITESPARSE_FOUND)
  # Do not pass VERSION_VAR to FindPackageHandleStandardArgs() if we failed to
  # find SuiteSparse to avoid a confusing autogenerated failure message
  # that states 'not found (missing: FOO) (found version: x.y.z)'.
  find_package_handle_standard_args(SuiteSparse
    REQUIRED_VARS ${SUITESPARSE_FOUND_REQUIRED_VARS}
    FAIL_MESSAGE "Failed to find some/all required components of SuiteSparse.")
endif (SUITESPARSE_FOUND)
