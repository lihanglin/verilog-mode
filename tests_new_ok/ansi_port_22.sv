interface ansi_port_22_if
#(
    parameter int unsigned BUS_W = 32
)
(
    `include "common_if_ports.svh"
    input  logic                   clk,
`ifndef NO_RST
    /* active low reset
     for interface */
    input  logic                   rst_n,
`else
    input  logic       [1:0]       por_state,
`endif
    output logic [3:0] [BUS_W-1:0] bus_data_o
);
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
