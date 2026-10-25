module inst_port_19;
`ifdef ARCH_DUAL_ISSUE
    alu_pipe u_alu0 (
        .clk     (clk),
        .op_a_i  ({a_hi[15:0], a_lo[15:0]}),
 `ifdef ALU_FLAGS_EN
        /* Arithmetic flag outputs */
        .flags_o (alu0_flags[3:0]),
 `endif
        .res_o   (alu0_res[31:0])
    );

    alu_pipe u_alu1 (
        .clk    (clk),
        .op_a_i ({b_hi[15:0], b_lo[15:0]}),
        .res_o  (alu1_res[31:0])
    );
`else
    alu_single u_alu_single (
        .clk          (clk),
        .op_single_i  (a_lo[15:0]),
        .res_single_o (alu0_res[15:0])
    );
`endif
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
