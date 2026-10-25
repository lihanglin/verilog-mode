module param_align_01 #(
      parameter int unsigned NUM_LANES = 4, // Active lanes
    parameter logic [3:0] [7:0] DEFAULT_MASK = 32'hFFFF_0000, // Lane mask
`ifdef EXTRA_PARAM
        parameter bit [15:0] EXTRA_CFG = 16'h0001, // Extra config
`endif
       localparam int unsigned LANE_BITS = $clog2(NUM_LANES) // Derived width
) (
    input  logic clk,
    output logic ready_o
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
