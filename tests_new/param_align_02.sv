module param_align_02;
    // Contiguous parameter & localparam declarations
      parameter int DATA_W = 64; // Data width
       localparam logic [1:0] [7:0] INIT_SEED = 16'hA55A; // Seed
`ifdef USE_PARITY
    localparam bit [7:0] PAR_MASK = 8'hFF;
`endif
     localparam int unsigned TOTAL_W = DATA_W + 8; // Total width
    // Adjacent signal declarations (must not merge with parameter block)
      logic [DATA_W-1:0] data_q;
    logic valid_q;
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
