typedef union packed {
    logic signed [31:0] s_word;
    logic unsigned [31:0] u_word;
    logic [3:0][7:0] bytes;
} alu_operand_u;
// Local Variables:
// verilog-align-decl-dimension: t
// verilog-align-decl-sub-dimension: t
// End:
