module top;
   alu_core u_alu (
      .clk_i(clk),
      . operand_a_i ({sign_ext, data_a[15:0]}),
      .operand_b_i(data_b[31:0]),
      .result_valid_o(alu_vld)
   );
endmodule
// Local Variables:
// verilog-align-decl-dimension: t
// End:
