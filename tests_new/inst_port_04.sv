module dual_stage;
   stage_a u_stage0 (
      .clk(clk),
      .in_valid_i(s0_vld),
      .out_data_o(s0_data)
   );

   stage_b u_stage1 (
      .clk(clk),
      .upstream_ack_o(s1_ack),
      .downstream_payload_o(s1_payload)
   );
endmodule
// Local Variables:
// verilog-align-decl-dimension: t
// End:
