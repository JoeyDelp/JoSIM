function(add_integration_test)
  set(option_args WILL_FAIL)
  set(single_args NAME CIR OUT)
  set(multi_args OVERWRITE_ARGS)
  cmake_parse_arguments(TEST
                        "${option_args}"
                        "${single_args}"
                        "${multi_args}"
                        ${ARGN})

  set(TEST_BUILD_DIR "${PROJECT_BINARY_DIR}/tests/integration/${TEST_NAME}")

  set(JOSIM_COMMAND josim-cli)

  set(JOSIM_COMMAND ${JOSIM_COMMAND} "-m" "1")

  if(DEFINED TEST_OUT)
    set(JOSIM_COMMAND ${JOSIM_COMMAND} "-o" "${TEST_OUT}")
  endif()


  if(DEFINED TEST_CIR)
    configure_file("${PROJECT_SOURCE_DIR}/test/${TEST_CIR}"
                   "${TEST_BUILD_DIR}/${TEST_CIR}")
    set(JOSIM_COMMAND ${JOSIM_COMMAND} "${TEST_BUILD_DIR}/${TEST_CIR}")
  endif()

  if(DEFINED TEST_OVERWRITE_ARGS)
    set(JOSIM_COMMAND JoSIM ${TEST_OVERWRITE_ARGS})
  else()

  endif()

  add_test(NAME "integration::${TEST_NAME}"
           WORKING_DIRECTORY "${TEST_BUILD_DIR}"
           COMMAND ${JOSIM_COMMAND})

  if(${TEST_WILL_FAIL})
    set_tests_properties("integration::${TEST_NAME}" PROPERTIES WILL_FAIL TRUE)
  endif()

endfunction()
