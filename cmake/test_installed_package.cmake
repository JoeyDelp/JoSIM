function(run_checked)
  execute_process(COMMAND ${ARGV}
                  RESULT_VARIABLE result
                  OUTPUT_VARIABLE output
                  ERROR_VARIABLE error)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "Installed-package check failed (${result}):\n${output}\n${error}")
  endif()
endfunction()

if(NOT TEST_BINARY_DIR OR NOT TEST_SOURCE_DIR)
  message(FATAL_ERROR "Installed-package tests require source and build directories")
endif()
# This directory contains only this test's generated installation and consumer.
file(REMOVE_RECURSE "${TEST_BINARY_DIR}/installed-consumer")
set(prefix "${TEST_BINARY_DIR}/installed-consumer/prefix")
set(consumer_build "${TEST_BINARY_DIR}/installed-consumer/build")
run_checked("${CMAKE_COMMAND}" --install "${TEST_BINARY_DIR}" --prefix "${prefix}" --config "${TEST_CONFIG}")
# Remove the original prefix before discovery, so stale paths cannot pass.
set(relocated_prefix "${TEST_BINARY_DIR}/installed-consumer/relocated prefix")
file(RENAME "${prefix}" "${relocated_prefix}")
set(prefix "${relocated_prefix}")
foreach(guide compatibility.md sweeps.md model_limits.md performance.md)
  if(NOT EXISTS "${prefix}/docs/${guide}")
    message(FATAL_ERROR "Installed manual is missing ${guide}")
  endif()
endforeach()
if(EXISTS "${prefix}/docs/context")
  message(FATAL_ERROR "Ignored local project context was installed")
endif()

set(configure_args -S "${TEST_SOURCE_DIR}/test/consumer" -B "${consumer_build}"
                   -G "${TEST_GENERATOR}" "-DCMAKE_PREFIX_PATH=${prefix}"
                   "-Djosim_DIR=${prefix}/share/josim"
                   -DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF
                   -DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF
                   "-DCMAKE_CXX_COMPILER=${TEST_CXX_COMPILER}")
if(TEST_MSVC_RUNTIME_LIBRARY)
  list(APPEND configure_args "-DCMAKE_MSVC_RUNTIME_LIBRARY=${TEST_MSVC_RUNTIME_LIBRARY}")
endif()
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
set(executable_suffix "")
if(WIN32)
  set(executable_suffix ".exe")
  # DLL discovery for this test process only; no machine/user PATH changes.
  set(ENV{PATH} "${prefix}/bin;$ENV{PATH}")
endif()
run_checked("${prefix}/bin/josim-cli${executable_suffix}" -v)
run_checked("${CMAKE_CTEST_COMMAND}" --test-dir "${consumer_build}" -C "${TEST_CONFIG}" --output-on-failure)
