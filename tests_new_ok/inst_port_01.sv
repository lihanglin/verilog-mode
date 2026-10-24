my_submodule u_sub (
    .clk        (sys_clk),
    .rst_n      (sys_rst_n),
    .data_in_dp (wide_data_bus[31:0]),
    .ready_o    (sub_ready)
);
// Local Variables:
// verilog-align-decl-dimension: t
// End:
