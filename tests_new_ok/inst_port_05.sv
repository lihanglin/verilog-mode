module pad_ring;
   io_pad u_pad_clk (
      .pad_io     (pad_clk),  // external pin
      .oen_i      (1'b1),     // input only
      .din_i      (1'b0),
      .dout_o     (core_clk), // to pll
      .test_obs_o ()          // unused
   );
endmodule
// Local Variables:
// verilog-align-decl-dimension: t
// End:
