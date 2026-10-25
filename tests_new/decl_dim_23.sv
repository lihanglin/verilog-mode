module decl_dim_23;
   // Declarations with macro types and macro dimensions
       logic [`LANE_W-1:0] lane_vld;
`ifdef USE_DEC_PKT_MACRO
   /* Macro type declaration
      with two packed dimensions */
  `DEC_PKT_T [3:0] [`LANE_W-1:0] lane_pkt;
`else
     logic [1:0] [`LANE_W-1:0] lane_raw;
`endif
      `UOP_INFO_T [`UOP_W-1:0] uop_info;
   logic uop_rdy;
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
