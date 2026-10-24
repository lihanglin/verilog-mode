module my_unit (
    input  logic                           clk,
    input  idu_opcode_e             [3:0]  opcode_i,
    output logic        [NSvdq-1:0] [63:0] payload_o
);
endmodule
// Local Variables:
// verilog-align-decl-dimension: t
// verilog-align-decl-sub-dimension: t
// End:
