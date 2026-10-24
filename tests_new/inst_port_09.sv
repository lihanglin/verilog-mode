module expr_inst;
   pkt_mux u_mux (
      .clk(clk),
      .sel_i(bypass_en ? 2'b00 : mux_sel[1:0]),
      .data_in_i({4{lane_word[15:0]}}),
      .selected_pkt_o(mux_out_pkt)
   );
endmodule
// Local Variables:
// verilog-align-decl-dimension: t
// End:
