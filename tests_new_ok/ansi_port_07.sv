module lsu_pipe (
    input  logic                           clk,
    input  lsu_pkg::lsu_req_t        [1:0] req_i,
    output lsu_pkg::lsu_resp_t [3:0] [1:0] resp_o,
    output logic                           flush_o
);
endmodule
// Local Variables:
// verilog-align-decl-dimension: t
// verilog-align-decl-sub-dimension: t
// End:
