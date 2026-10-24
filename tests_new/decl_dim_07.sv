module signed_decls;
   logic signed [WIDTH-1:0] acc_data;
   logic unsigned [BANKS-1:0][WIDTH-1:0] bank_data;
   alu_flag_t [WIDTH-1:0] flags;
   logic ovf_flag;
endmodule
// Local Variables:
// verilog-align-decl-dimension: t
// verilog-align-decl-sub-dimension: t
// End:
