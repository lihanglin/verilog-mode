class enum_align_02_c;
    typedef enum bit [3:0] {
        // Basic commands
          CMD_NOP = 4'h0,
       CMD_READ,
`ifdef ENABLE_BURST_CMD
        /* Burst mode
           operations */
            CMD_BURST_RD = 4'h4,
      CMD_BURST_WR = 4'h5,
`else
         CMD_SINGLE_WR = 4'h2,
`endif
          CMD_ABORT = 4'hF
    } bus_cmd_e;
endclass
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
