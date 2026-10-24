module fifo_wrap;
   sync_fifo #(.WIDTH(64), .DEPTH(16)) u_sync_fifo (
      .clk         (clk_i),
      .rst_n       (rst_ni),
      .push_en_i   (fifo_push),
      .push_data_i (fifo_wdata),
      .pop_en_i    (fifo_pop),
      .pop_data_o  (fifo_rdata),
      .full_o      (fifo_full),
      .empty_o     (fifo_empty)
   );
endmodule
// Local Variables:
// verilog-align-decl-dimension: t
// End:
