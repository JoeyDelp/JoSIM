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