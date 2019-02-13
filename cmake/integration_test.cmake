function(add_integration_test)
  set(option_args)
  set(single_args NAME CIR OUT OVERWRITE_ARGS)
  set(multi_args)
  cmake_parse_arguments(TEST "${option_args}" "${single_args}" "${multi_args}" ${ARGN})

  set(TEST_BUILD_DIR "${CMAKE_BINARY_DIR}/tests/integration/${TEST_NAME}")
  configure_file("${CMAKE_SOURCE_DIR}/test/${TEST_CIR}" "${TEST_BUILD_DIR}/${TEST_CIR}")

  set(JOSIM_COMMAND JoSIM)

  if(DEFINED TEST_OUT)
    set(JOSIM_COMMAND ${JOSIM_COMMAND} "-o" "${TEST_OUT}")
  endif()

  set(JOSIM_COMMAND ${JOSIM_COMMAND} "${TEST_CIR}")

  if(DEFINED TEST_OVERWRITE_ARGS)
    set(JOSIM_COMMAND ${JOSIM_COMMAND} ${TEST_OVERWRITE_ARGS})
  endif()

  add_test(
    NAME "integration::${TEST_NAME}"
    WORKING_DIRECTORY "${TEST_BUILD_DIR}"
    COMMAND ${JOSIM_COMMAND}
  )
endfunction()
