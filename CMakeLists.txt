# Specify the minimum version for CMake
cmake_minimum_required(VERSION 3.10)
# Project name
project(JoSIM)
# The version number.
set(JoSIM_VERSION_MAJOR 1)
set(JoSIM_VERSION_MINOR 0)
# Ensure C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message("Platform:  ${CMAKE_SYSTEM_NAME}")
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(WINDOWS TRUE)
    set(PLATFORM "win")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(LINUX TRUE)
    set(PLATFORM "linux")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
    set(PLATFORM "mac")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/FL)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/suitesparse)

add_subdirectory(src)

find_package(FLTK REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/${PLATFORM}/${CMAKE_BUILD_TYPE})

add_executable(JoSIM ${JOSIM_SRC})

if(MACOSX OR LINUX)
    target_link_libraries(JoSIM ${FLTK_LIBRARIES})
    target_link_libraries(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libamd.a)
    target_link_libraries(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libbtf.a)
    target_link_libraries(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libcamd.a)
    target_link_libraries(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libccolamd.a)
    target_link_libraries(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libcholmod.a)
    target_link_libraries(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libcolamd.a)
    target_link_libraries(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libcxsparse.a)
    target_link_libraries(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libklu.a)
    target_link_libraries(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libldl.a)
    target_link_libraries(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libspqr.a)
    target_link_libraries(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libsuitesparseconfig.a)
    target_link_libraries(JoSIM ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}/libumfpack.a)
endif()
