module mixed_inst;
   manual_mod u_man (
      .clk(clk),
      .long_port_name(sig_a)
   );

   auto_mod u_auto (/*AUTOINST*/
                    // Outputs
                    .out_a(out_a),
                    // Inputs
                    .in_long_b(in_long_b));
endmodule
// Local Variables:
// verilog-align-decl-dimension: t
// End:
