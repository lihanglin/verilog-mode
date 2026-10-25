interface decl_dim_17_if #(
    parameter int unsigned W = 64
) (
    input logic clk,
    input logic rst_n
);
    // Interface internal declarations
    logic                   req_vld;
`ifndef DISABLE_PARITY
    /* Parity and ECC signals
     for interface payload */
    logic     [3:0] [7:0]   req_ecc;
`else
    logic           [7:0]   req_par;
`endif
    bus_hdr_t [1:0] [W-1:0] req_hdr;
    wire            [W-1:0] req_data;
endinterface
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
