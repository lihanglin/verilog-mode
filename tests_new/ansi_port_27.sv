module ansi_port_27
      (
   // Macro type and macro width ports
       input logic [`BUS_W-1:0] bus_in,
`ifdef USE_MACRO_PKT
   /* Macro packet type with unpacked/packed dims */
     input `PKT_T [3:0] [`BUS_W-1:0] pkt_in,
`else
  input logic [1:0] [`BUS_W-1:0] raw_in,
`endif
      output `RESP_T [`RESP_W-1:0] resp_out
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
