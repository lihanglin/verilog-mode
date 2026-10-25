module inst_port_16;
    axi_crossbar
    #(
        // Crossbar parameter configuration
        .NUM_MASTERS (4),
`ifdef AXI_LARGE_SLAVES
        /* Large slave count
         and address width */
        .NUM_SLAVES  (16),
        .ADDR_WIDTH  (64)
`else
        .NUM_SLAVES  (8),
        .ADDR_WIDTH  (32)
`endif
    )
    u_xbar
    (
        // Global clock and reset
        .aclk      (axi_clk),
        .aresetn   (axi_rst_n),
`ifdef AXI_USER_SIGNALS
        /* Optional AXI user sideband */
        .awuser_i  (m_awuser),
`endif
        .awvalid_i (m_awvalid),
        .awready_o (m_awready)
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
