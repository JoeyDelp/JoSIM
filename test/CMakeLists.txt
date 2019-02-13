include(${CMAKE_SOURCE_DIR}/cmake/integration_test.cmake)

add_integration_test(
  NAME basic_jtl
  CIR basic_jtl.cir
  OUT basic_jtl.csv
)

add_integration_test(
  NAME dcsfq_jtl_sink
  CIR dcsfq_jtl_sink.cir
  OUT dcsfq_jtl_sink.csv
)

add_integration_test(
  NAME test_mitll_dfft
  CIR TEST_MITLL_DFFT.cir
  OUT test_mitll_dfft.csv
)

add_integration_test(
  NAME test_mitll_xort
  CIR TEST_MITLL_XORT.cir
  OUT test_mitll_xort.csv
)
