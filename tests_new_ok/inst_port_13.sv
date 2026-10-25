module inst_port_13;
    my_submodule u_sub
    (
        // Clock and reset connections
        .clk            (sys_clk),
        .rst_n          (sys_rst_n),
`ifdef SUB_WIDE_BUS
        /* Wide bus connections
         spanning two lines */
        .data_in_dp     (wide_data_bus[63:0]),
        .valid_in_dp    (wide_valid), // valid comment
`elsif SUB_MED_BUS
        .data_med_dp    (med_data_bus[31:0]),
`else
        .data_narrow_dp (narrow_data_bus[15:0]),
`endif
        // Final ready connection
        .ready_o        (sub_ready)
    );
endmodule
// Local Variables:
// verilog-indent-lists: nil
// verilog-indent-level: 4
// verilog-indent-level-module: 4
// verilog-indent-level-declaration: 4
// verilog-indent-level-behavioral: 4
// verilog-cexp-indent: 4
// verilog-case-indent: 4
// indent-tabs-mode: nil
// verilog-align-decl-dimension: t
// verilog-align-decl-sub-dimension: t
// verilog-auto-newline: nil
// verilog-auto-lineup-on-semi: nil
// End:
