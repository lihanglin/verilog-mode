module ansi_port_20
(
    // Clock and reset
    input  logic                   clk,
    input  logic                   rst_n,
`ifdef FEAT_A
    /* Multi-line block comment
     describing feat A inputs */
    input  logic      [3:0] [7:0]  feat_a_data,
    input  feat_pkt_t       [7:0]  feat_a_pkt, // pkt comment
`elsif FEAT_B
    input  logic            [15:0] feat_b_data,
`else
    input  logic            [7:0]  feat_def_data,
`endif
    // Final output port
    output logic      [1:0] [31:0] result_o
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
