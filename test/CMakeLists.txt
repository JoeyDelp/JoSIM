# Commad line options

#add_integration_test(
#  NAME output_after_cir
#  CIR basic_jtl.cir
#  OUT basic_jtl.csv
#  OVERWRITE_ARGS basic_jtl.cir -o basic_jtl.csv
#)

# Tests
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
  WRSPICE
)

add_integration_test(
  NAME modified_netlist
  CIR modified_netlist.cir
  OUT modified_netlist.csv
  WRSPICE
)

#add_integration_test(
#  NAME test_mitll_xort
#  CIR TEST_MITLL_XORT.cir
#  OUT test_mitll_xort.csv
#  WRSPICE
#)
