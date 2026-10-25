module inst_port_15;
    fifo_ctrl #(
      // Depth and width parameters
        .DEPTH(16),
`ifdef FIFO_WIDE_PARAM
      /* Wide parameter override
         with ECC support */
   .DATA_WIDTH(128),
       .ECC_EN(1'b1)
`else
     .DATA_WIDTH(64),
  .ECC_EN(1'b0)
`endif
    ) u_fifo (
       .clk(clk),
`ifndef FIFO_NO_FLUSH
      /* Flush control port */
   .flush_i(fifo_flush),
`endif
        .push_data_i(push_data),
     .pop_data_o(pop_data)
    );
endmodule
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
