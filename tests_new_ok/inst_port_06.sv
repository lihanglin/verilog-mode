module lane_array;
   for (genvar g = 0; g < 4; g++) begin : gen_lanes
      serdes_lane u_lane (
         .refclk_i        (refclk),
         .rx_p_i          (rx_p[g]),
         .rx_n_i          (rx_n[g]),
         .parallel_data_o (lane_data[g])
      );
   end
endmodule
// Local Variables:
// verilog-align-decl-dimension: t
// End:
