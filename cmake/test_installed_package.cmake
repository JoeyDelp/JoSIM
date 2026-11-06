function(run_checked)
  execute_process(COMMAND ${ARGV}
                  RESULT_VARIABLE result
                  OUTPUT_VARIABLE output
                  ERROR_VARIABLE error)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "Installed-package check failed (${result}):\n${output}\n${error}")
  endif()
endfunction()

set(prefix "${TEST_BINARY_DIR}/installed-consumer/prefix")
set(consumer_build "${TEST_BINARY_DIR}/installed-consumer/build")
run_checked("${CMAKE_COMMAND}" --install "${TEST_BINARY_DIR}" --prefix "${prefix}" --config "${TEST_CONFIG}")

set(configure_args -S "${TEST_SOURCE_DIR}/test/consumer" -B "${consumer_build}"
                   -G "${TEST_GENERATOR}" "-DCMAKE_PREFIX_PATH=${prefix}"
                   "-DCMAKE_CXX_COMPILER=${TEST_CXX_COMPILER}")
if(TEST_PLATFORM)
  list(APPEND configure_args -A "${TEST_PLATFORM}")
endif()
if(TEST_OSX_ARCHITECTURES)
  list(APPEND configure_args "-DCMAKE_OSX_ARCHITECTURES=${TEST_OSX_ARCHITECTURES}")
endif()
if(TEST_OSX_DEPLOYMENT_TARGET)
  list(APPEND configure_args "-DCMAKE_OSX_DEPLOYMENT_TARGET=${TEST_OSX_DEPLOYMENT_TARGET}")
endif()
run_checked("${CMAKE_COMMAND}" ${configure_args})
run_checked("${CMAKE_COMMAND}" --build "${consumer_build}" --config "${TEST_CONFIG}")
run_checked("${CMAKE_CTEST_COMMAND}" --test-dir "${consumer_build}" -C "${TEST_CONFIG}" --output-on-failure)
