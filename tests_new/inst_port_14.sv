module inst_port_14;
    idu_dec_slice u_dec_slice
         (
       .clk(clk),
   .rst_n(rst_n),
        .instr_i(instr_dp[31:0]),
`ifdef DEC_FULL_TRACE
      /* Full trace output port without trailing comma */
     .trace_full_pkt_o(trace_full_pkt)
`elsif DEC_MINI_TRACE
   .trace_mini_o(trace_mini)
`else
      // Fallback status output port without trailing comma
        .dec_status_o(dec_status)
`endif
       );
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
// verilog-auto-newline: nil
// verilog-auto-lineup-on-semi: nil
// End:
