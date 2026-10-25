module decl_dim_20;
    task automatic run_burst_transfer
          (
    input logic [15:0] base_addr,
       output logic transfer_done
        );
      // Task local variables with directives
         logic [3:0] [15:0] beat_addr;
`ifndef BURST_SINGLE_BEAT
      /* Multi-beat counters
         and status flags */
    burst_info_t [1:0] [7:0] beat_info;
`else
        logic [7:0] single_info;
`endif
     logic beat_vld;

        beat_vld      = 1'b1;
        transfer_done = beat_vld;
    endtask
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
