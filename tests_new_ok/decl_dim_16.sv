module decl_dim_16;
    // Contiguous signal declarations with directives and comments
    logic                [3:0] [NSvdq-1:0] svdq_vld_dp;
`ifdef ENABLE_EXT_PKT
    /* Multi-line block comment
     inside contiguous declaration block */
    idu_svdq_pkt_t             [NSvdq-1:0] svdq_pkt_dp;
 `define LOCAL_SVDQ_TAG 1
    logic          [1:0] [3:0] [NSvdq-1:0] svdq_rdy_dp;
 `undef LOCAL_SVDQ_TAG
`elsif ENABLE_COMPACT_PKT
    compact_pkt_t              [3:0]       svdq_compact_dp;
`else
    logic                      [NSvdq-1:0] svdq_fallback_dp;
`endif
    // Final signal in group
    logic                                  svdq_done_dp;
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
