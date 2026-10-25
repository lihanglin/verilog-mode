class decl_dim_18_c extends uvm_sequence_item;
    // Class property declarations with qualifiers and directives
    rand logic         [3:0] [15:0] addr_vec;
`ifdef RAND_EXT_MODE
    /* Cyclic random opcode and
     local transaction id */
    randc bit                [7:0]  opcode;
    local int unsigned              txn_id;
`else
    protected logic          [15:0] fallback_id;
`endif
    // Static counter and packet member
    static bit               [31:0] total_cnt;
    idu_pkt_t          [1:0] [7:0]  payload_pkt;
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
