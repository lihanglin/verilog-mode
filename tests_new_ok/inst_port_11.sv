module inst_allman_top;

    sub_mod u_simple
    (
        .clk           (clk),
        .wide_data_bus (data_bus)
    );

    param_fifo
    #(
        .WIDTH      (64),
        .DEPTH_LOG2 (5)
    )
    u_fifo_a
    (
        .clk_i       (clk),
        .push_data_i (push_data)
    );

    param_fifo #
    (
        .WIDTH      (32),
        .DEPTH_LOG2 (4)
    )
    u_fifo_b
    (
        .clk_i       (clk),
        .push_data_i (push_data_b)
    );

endmodule
// Local Variables:
// verilog-indent-level: 4
// verilog-indent-level-module: 4
// verilog-indent-level-declaration: 4
// verilog-cexp-indent: 4
// verilog-align-decl-dimension: t
// verilog-align-decl-sub-dimension: t
// indent-tabs-mode: nil
// End:
