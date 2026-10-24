module dsp_mac (
    input  logic               clk,
    input  logic signed [17:0] coeff_a_i,
    input  logic signed [24:0] sample_b_i,
    output logic signed [47:0] accum_o
);
endmodule
// Local Variables:
// verilog-align-decl-dimension: t
// End:
