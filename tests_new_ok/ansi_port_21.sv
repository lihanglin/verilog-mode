module ansi_port_21
(
    input  logic                   clk,
    input  logic      [3:0] [15:0] req_bus,
`ifdef USE_WIDE_RESP
    /* Wide response branch without trailing comma */
    output logic      [1:0] [63:0] resp_wide_o
`elsif USE_MED_RESP
    output resp_med_t       [31:0] resp_med_o
`else
    // Narrow response fallback
    output logic            [15:0] resp_narrow_o
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
