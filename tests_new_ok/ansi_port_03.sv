module axi_bridge #(parameter int WIDTH = 64) (
    input  wire                    aclk,
    input  wire                    aresetn,
    inout  tri         [7:0]       gpio_io,
    input  logic       [WIDTH-1:0] s_wdata_i,
    output logic [3:0] [WIDTH-1:0] m_wdata_o
);
endmodule
// Local Variables:
// verilog-align-decl-dimension: t
// verilog-align-decl-sub-dimension: t
// End:
