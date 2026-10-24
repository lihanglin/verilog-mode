module regfile_2r1w (
    input logic clk, // system clock
    input logic [4:0] raddr_i [0:1], // read addresses
    output logic [63:0] rdata_o [0:1] // read data
);
endmodule
// Local Variables:
// verilog-align-decl-dimension: t
// End:
