module pkg_typed_decls;
   axi_pkg::aw_chan_t             [1:0] aw_ch;
   axi_pkg::w_chan_t        [3:0] [1:0] w_ch;
   logic              [7:0] [3:0] [1:0] wstrb_pipe;
   logic                                bready;
endmodule
// Local Variables:
// verilog-align-decl-dimension: t
// verilog-align-decl-sub-dimension: t
// End:
