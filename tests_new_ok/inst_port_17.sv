module inst_port_17;
`include "gcpu_smu_idu_subm_hdr.svh"
    gcpu_smu_idu_subm u_subm
    (
        `include "gcpu_smu_idu_subm_ports.svh"
        .clk       (clk),
`ifdef SUBM_DBG_PORT
        /* Debug bus connection */
        .dbg_bus_i (dbg_bus[31:0]),
`endif
        .dec_vld_o (dec_vld)
    );
`include "gcpu_inst_end.svh"
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
