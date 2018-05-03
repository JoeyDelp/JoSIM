# Welcoming MESSAGE
MESSAGE("Welcome to the JoSIM CMake file.")
MESSAGE("Please wait while we SET a few configuration options.")
MESSAGE("=====================================================")
# IdentIFy the platform type
MESSAGE("Platform:  ${CMAKE_SYSTEM_NAME}")
# IdentIFy the build type
IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
    MESSAGE("Build type: ${CMAKE_BUILD_TYPE}")
ELSE()
    # Default build type is release. Change to debug for debugging
    SET(CMAKE_BUILD_TYPE RELEASE)
    MESSAGE("Build type: ${CMAKE_BUILD_TYPE}")
ENDIF()
# SpecIFy the minimum version for CMake
CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
# Project name
PROJECT(JoSIM)
# The version number.
SET(JoSIM_VERSION_MAJOR 1)
SET(JoSIM_VERSION_MINOR 0)
# Ensure C++17
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
# Include directory for JoSIM header files
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)

IF (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    SET(WINDOWS TRUE)
    SET(PLATFORM "win")
	INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include/Fl)
	INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include/suitesparse)
ELSEIF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(LINUX TRUE)
    SET(PLATFORM "linux")
    # SpecIFy path to custom FindSuiteSparse.cmake file
    SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/lib/${PLATFORM}/")
    FIND_PACKAGE(FLTK REQUIRED)
    FIND_PACKAGE(SuiteSparse REQUIRED)
	INCLUDE_DIRECTORIES(${FLTK_INCLUDE_DIR})
	INCLUDE_DIRECTORIES(${SUITESPARSE_INCLUDE_DIRS})
ELSEIF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SET(MACOSX TRUE)
    SET(PLATFORM "mac")
    FIND_PACKAGE(FLTK REQUIRED)
	INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include/suitesparse)
	INCLUDE_DIRECTORIES(${FLTK_INCLUDE_DIR})
ENDIF()

# Add the source files for JoSIM
ADD_SUBDIRECTORY(src)

# Add the source to the executable for compilation
ADD_EXECUTABLE(JoSIM ${JOSIM_SRC})

# IF Darwin system FLTK can be installed from source and found but SuiteSparse is harder.
# We therefore provide SuiteSparse libraries as Darwin is not as fragmented and libraries can be precompiled
IF(MACOSX)
    TARGET_LINK_LIBRARIES(JoSIM ${FLTK_LIBRARIES})
    TARGET_LINK_LIBRARIES(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libamd.a)
    TARGET_LINK_LIBRARIES(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libbtf.a)
    TARGET_LINK_LIBRARIES(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libcamd.a)
    TARGET_LINK_LIBRARIES(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libccolamd.a)
    TARGET_LINK_LIBRARIES(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libcholmod.a)
    TARGET_LINK_LIBRARIES(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libcolamd.a)
    TARGET_LINK_LIBRARIES(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libcxsparse.a)
    TARGET_LINK_LIBRARIES(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libklu.a)
    TARGET_LINK_LIBRARIES(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libldl.a)
    TARGET_LINK_LIBRARIES(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libspqr.a)
    TARGET_LINK_LIBRARIES(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libsuitesparseconfig.a)
    TARGET_LINK_LIBRARIES(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libumfpack.a)
ELSEIF(LINUX)
	TARGET_LINK_LIBRARIES(JoSIM ${FLTK_LIBRARIES})
	TARGET_LINK_LIBRARIES(JoSIM ${SUITESPARSE_LIBRARIES})
ENDIF()

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME}_${PLATFORM}_${CMAKE_BUILD_TYPE})

MESSAGE("=========================================================")
MESSAGE("Configuration done.")
MESSAGE("Now run make")