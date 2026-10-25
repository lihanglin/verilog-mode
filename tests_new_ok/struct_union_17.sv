class struct_union_17_c extends uvm_scoreboard;
    typedef struct packed
    {
        `include "zrf_common_fields.svh"
        // Scoreboard entry fields
        bit                       valid;
`ifdef SB_TRACK_PC
        /* Program counter and
         branch mask */
        logic [1:0] [63:0]        pc_val;
`else
        logic       [31:0]        pc_low;
`endif
        bit         [WXuopId-1:0] xuopid;
    } sb_entry_t;
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
