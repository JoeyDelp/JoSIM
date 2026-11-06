if(NOT TEST_SOURCE_DIR OR NOT TEST_BINARY_DIR)
  message(FATAL_ERROR "Build-option checks require source and build directories")
endif()
set(arguments -S "${TEST_SOURCE_DIR}"
              -G "${TEST_GENERATOR}"
              "-DCMAKE_C_COMPILER=${TEST_C_COMPILER}"
              "-DCMAKE_CXX_COMPILER=${TEST_CXX_COMPILER}"
              "-DFETCHCONTENT_SOURCE_DIR_CMAKESUITESPARSE=${TEST_SUITESPARSE_SOURCE}")
if(TEST_PLATFORM)
  list(APPEND arguments -A "${TEST_PLATFORM}")
endif()

# Defining the retired macro outside CMake must not select a different layout.
set(macro_source "${TEST_BINARY_DIR}/rejected-options/macro-source")
set(macro_build "${TEST_BINARY_DIR}/rejected-options/macro-build")
file(MAKE_DIRECTORY "${macro_source}")
file(WRITE "${macro_source}/main.cpp"
  "#include <JoSIM/TypeDefines.hpp>\n#include <JoSIM/LUSolve.hpp>\nint main() {}\n")
file(WRITE "${macro_source}/CMakeLists.txt"
  "cmake_minimum_required(VERSION 3.14)\nproject(RetiredMacro LANGUAGES CXX)\nadd_executable(probe main.cpp)\ntarget_include_directories(probe PRIVATE \"${TEST_SOURCE_DIR}/include\")\ntarget_compile_features(probe PRIVATE cxx_std_17)\ntarget_compile_definitions(probe PRIVATE SLU)\n")
set(macro_arguments -S "${macro_source}" -B "${macro_build}"
                    -G "${TEST_GENERATOR}"
                    "-DCMAKE_CXX_COMPILER=${TEST_CXX_COMPILER}")
if(TEST_PLATFORM)
  list(APPEND macro_arguments -A "${TEST_PLATFORM}")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" ${macro_arguments}
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error
                TIMEOUT 30)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "Macro probe configuration failed:\n${output}\n${error}")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${macro_build}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error
                TIMEOUT 30)
file(WRITE "${TEST_BINARY_DIR}/rejected-options/macro.log" "${output}\n${error}")
if(result EQUAL 0 OR NOT "${output}\n${error}" MATCHES "SuperLU backend has been retired")
  message(FATAL_ERROR "SLU macro was not explicitly rejected (${result}):\n${output}\n${error}")
endif()
foreach(option IN ITEMS SLU USING_OPENMP)
  if(option STREQUAL "SLU")
    set(diagnostic "SuperLU backend has been retired")
  else()
    set(diagnostic "Parallel processing is unsupported")
  endif()
  execute_process(COMMAND "${CMAKE_COMMAND}" ${arguments}
                  -B "${TEST_BINARY_DIR}/rejected-options/${option}" "-D${option}=ON"
                  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error
                  TIMEOUT 45)
  file(MAKE_DIRECTORY "${TEST_BINARY_DIR}/rejected-options")
  file(WRITE "${TEST_BINARY_DIR}/rejected-options/${option}.log" "${output}\n${error}")
  if(result EQUAL 0 OR NOT "${output}\n${error}" MATCHES "${diagnostic}")
    message(FATAL_ERROR "${option}=ON was not explicitly rejected (${result}):\n${output}\n${error}")
  endif()
endforeach()
