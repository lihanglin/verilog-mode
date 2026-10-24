module xbar_switch (
    input  logic                    clk,
    input  logic             [7:0]  port_en_i,
    input  logic       [3:0] [7:0]  route_id_i,
    output logic [1:0] [3:0] [31:0] flit_data_o
);
endmodule
// Local Variables:
// verilog-align-decl-dimension: t
// verilog-align-decl-sub-dimension: t
// End:
