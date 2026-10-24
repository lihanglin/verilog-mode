module param_mod_allman
   #(
    parameter int WIDTH = 8,
    parameter int LANES = 4
   )
   (
    input logic clk,
    input logic rst_n,
    input logic [LANES-1:0] [WIDTH-1:0] data_i,
    output logic [WIDTH-1:0] data_o
   );

    assign data_o = data_i[0];

endmodule
// Local Variables:
// verilog-indent-lists: nil
// verilog-indent-level: 4
// verilog-indent-level-module: 4
// verilog-indent-level-declaration: 4
// verilog-cexp-indent: 4
// verilog-align-decl-dimension: t
// verilog-align-decl-sub-dimension: t
// indent-tabs-mode: nil
// End:
