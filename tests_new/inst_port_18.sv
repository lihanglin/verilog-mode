module inst_port_18 #(
    parameter int N_LANES = 2
);
    generate
        for (genvar i = 0; i < N_LANES; i++) begin : gen_lane
            lane_worker #(
               .LANE_ID(i),
`ifdef LANE_WIDE_MODE
             .LANE_WIDTH(64)
`else
                .LANE_WIDTH(32)
`endif
            ) u_lane (
              // Per-lane connections
                 .clk(clk),
`ifdef LANE_WIDE_MODE
              /* Wide lane input and output */
            .data_in_i(lane_in[i][63:0]),
`else
               .data_in_i(lane_in[i][31:0]),
`endif
             .lane_ready_o(lane_rdy[i])
            );
        end
    endgenerate
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
