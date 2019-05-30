# Commad line options

#add_integration_test(
#  NAME output_after_cir
#  CIR basic_jtl.cir
#  OUT basic_jtl.csv
#  OVERWRITE_ARGS basic_jtl.cir -o basic_jtl.csv
#)

# Tests
add_integration_test(
  NAME ex_jtl_basic
  CIR ex_jtl_basic.cir
  OUT ex_jtl_basic.csv
)

add_integration_test(
  NAME ex_dcsfq_jtl_sink
  CIR ex_dcsfq_jtl_sink.cir
  OUT ex_dcsfq_jtl_sink.csv
)

add_integration_test(
  NAME ex_mitll_dff_wr
  CIR ex_mitll_dff_wr.cir
  OUT ex_mitll_dff_wr.csv
  WRSPICE
)

add_integration_test(
  NAME ex_mitll_not_wr
  CIR ex_mitll_not_wr.cir
  OUT ex_mitll_not_wr.csv
  WRSPICE
)

add_integration_test(
  NAME test_param_nest
  CIR test_param_nest.cir
  OUT test_param_nets.csv
)

add_integration_test(
  NAME test_param_undef
  CIR test_param_undef.cir
  WILL_FAIL
)

#add_integration_test(
#  NAME test_mitll_xort
#  CIR TEST_MITLL_XORT.cir
#  OUT test_mitll_xort.csv
#  WRSPICE
#)
