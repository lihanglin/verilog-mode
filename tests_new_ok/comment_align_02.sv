module comment_align_02 (
    input  logic             clk, // System clock
    input  logic [1:0] [7:0] data_i, // Input data
    output logic             ready_o // Ready output
);
    logic [3:0] [7:0] buf_q; // Buffer register
    logic             vld_q; // Valid bit
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
// verilog-align-decl-expr-comments: nil
// verilog-auto-newline: nil
// verilog-auto-lineup-on-semi: nil
// End:
