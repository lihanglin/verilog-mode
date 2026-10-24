module messy_inst;
   arb_rr u_arb (
      .clk           (sys_clk),
      .req_i         (req_vec[7:0]),
      .grant_valid_o (gnt_vld),
      .grant_id_o    (gnt_id[2:0])
   );
endmodule
// Local Variables:
// verilog-align-decl-dimension: t
// End:
