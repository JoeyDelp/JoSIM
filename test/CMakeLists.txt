# Commad line options

#add_integration_test(
#  NAME output_after_cir
#  CIR basic_jtl.cir
#  OUT basic_jtl.csv
#  OVERWRITE_ARGS basic_jtl.cir -o basic_jtl.csv
#)

# Tests

add_integration_test(
  NAME test_capacitor
  CIR comp/capacitor.cir
)

add_integration_test(
  NAME test_cccs
  CIR comp/cccs.cir
)

add_integration_test(
  NAME test_ccvs
  CIR comp/ccvs.cir
)

add_integration_test(
  NAME test_inductor
  CIR comp/inductor.cir
)

add_integration_test(
  NAME test_jj
  CIR comp/jj.cir
)

add_integration_test(
  NAME test_ps
  CIR comp/ps.cir
)

add_integration_test(
  NAME test_resistor
  CIR comp/resistor.cir
)

add_integration_test(
  NAME test_tx
  CIR comp/tx.cir
)

add_integration_test(
  NAME test_vccs
  CIR comp/vccs.cir
)

add_integration_test(
  NAME test_vcvs
  CIR comp/vcvs.cir
)

add_integration_test(
  NAME test_vs
  CIR comp/vs.cir
)

add_integration_test(
  NAME ex_dcsfq_jtl_sink
  CIR ex_dcsfq_jtl_sink.cir
  OUT ex_dcsfq_jtl_sink.csv
)

add_integration_test(
  NAME ex_gen_pp_ipht
  CIR ex_gen_pp_ipht.cir
  OUT ex_gen_pp_ipht.csv
)

add_integration_test(
  NAME ex_jtl_basic
  CIR ex_jtl_basic.cir
  OUT ex_jtl_basic.csv
)

add_integration_test(
  NAME ex_jtl_string
  CIR ex_jtl_string.cir
  OUT ex_jtl_string.csv
)

add_integration_test(
  NAME ex_ksa4bit
  CIR ex_ksa4bit.cir
  OUT ex_ksa4bit.csv
)

add_integration_test(
  NAME ex_mitll_dff_wr
  CIR ex_mitll_dff_wr.cir
  OUT ex_mitll_dff_wr.csv
)

add_integration_test(
  NAME ex_mitll_xor
  CIR ex_mitll_xor_wr.cir
  OUT ex_mitll_xor_wr.csv
)

add_integration_test(
  NAME test_param_nest
  CIR param/test_param_nest.cir
)

add_integration_test(
  NAME test_param_undef
  CIR param/test_param_undef.cir
  WILL_FAIL
)

add_integration_test(
  NAME test_line_cont
  CIR syntax/test_line_cont.cir
)

add_integration_test(
  NAME test_line_comment
  CIR syntax/test_line_comment.cir
)

add_integration_test(
  NAME test_output
  CIR syntax/test_output.cir
)

if(BUILD_TESTING)
  add_executable(josim-library-regression library_regression.cpp)
  target_link_libraries(josim-library-regression PRIVATE josim)
  josim_instrument_target(josim-library-regression)
  add_test(NAME regression::library COMMAND josim-library-regression)
  set_tests_properties(regression::library PROPERTIES TIMEOUT 30)
  add_test(NAME regression::build_options
    COMMAND ${CMAKE_COMMAND}
      "-DTEST_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
      "-DTEST_BINARY_DIR=${PROJECT_BINARY_DIR}"
      "-DTEST_GENERATOR=${CMAKE_GENERATOR}"
      "-DTEST_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
      "-DTEST_C_COMPILER=${CMAKE_C_COMPILER}"
      "-DTEST_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
      "-DTEST_SUITESPARSE_SOURCE=${CMakeSuiteSparse_SOURCE_DIR}"
      -P "${PROJECT_SOURCE_DIR}/cmake/test_rejected_options.cmake")
  set_tests_properties(regression::build_options PROPERTIES TIMEOUT 210)
  add_executable(josim-function-regression function_regression.cpp)
  target_link_libraries(josim-function-regression PRIVATE josim)
  josim_instrument_target(josim-function-regression)
  add_test(NAME regression::functions COMMAND josim-function-regression)
  set_tests_properties(regression::functions PROPERTIES TIMEOUT 30)
  add_executable(josim-noise-regression noise_regression.cpp)
  josim_instrument_target(josim-noise-regression)
  target_link_libraries(josim-noise-regression PRIVATE josim suitesparse::klu)
  add_test(NAME regression::noise COMMAND josim-noise-regression)
  set_tests_properties(regression::noise PROPERTIES TIMEOUT 90)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  add_test(NAME regression::gate
           COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/regression_gate.py
                   $<TARGET_FILE:josim-cli>)
  set_tests_properties(regression::gate PROPERTIES TIMEOUT 90)
  add_test(NAME regression::cli
           COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/regression_cli.py
                   $<TARGET_FILE:josim-cli>)
  set_tests_properties(regression::cli PROPERTIES TIMEOUT 90)
  add_test(NAME regression::installed_package
    COMMAND ${CMAKE_COMMAND}
      "-DTEST_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
      "-DTEST_BINARY_DIR=${PROJECT_BINARY_DIR}"
      "-DTEST_CONFIG=$<CONFIG>"
      "-DTEST_GENERATOR=${CMAKE_GENERATOR}"
      "-DTEST_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
      "-DTEST_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
      "-DTEST_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY}"
      "-DTEST_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}"
      "-DTEST_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}"
      -P "${PROJECT_SOURCE_DIR}/cmake/test_installed_package.cmake")
  set_tests_properties(regression::installed_package PROPERTIES TIMEOUT 90)
endif()
