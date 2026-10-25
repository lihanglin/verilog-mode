module assign_align_02 (
    input  logic [7:0] in_a,
    input  logic [7:0] in_b,
    output logic [7:0] out_sum,
    output logic       out_carry_flag,
    output logic [3:0] out_low_nibble
);
    // Continuous assign alignment when verilog-align-assign-expr is t
    assign out_sum        = in_a + in_b;           // 8-bit sum
`ifdef FAST_CARRY
    assign out_carry_flag = &(in_a | in_b);
`else
    assign out_carry_flag = (in_a + in_b) > 8'hFF; // Carry out
`endif
    assign out_low_nibble = in_a[3:0] & in_b[3:0]; // Low nibble mask
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
// verilog-auto-lineup: assignments
// verilog-align-assign-expr: t
// verilog-auto-newline: nil
// verilog-auto-lineup-on-semi: nil
// End:
