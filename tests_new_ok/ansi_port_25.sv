class ansi_port_25_c;
    virtual function automatic logic check_entry
    (
        // Index and tag inputs
        input  logic     [1:0] [11:0] vpn_idx,
`ifdef TLB_EXT_TAG
        /* Extended tag
         with ASID */
        input  tlb_tag_t [3:0] [15:0] ext_tag_in,
`else
        input  tlb_tag_t       [11:0] base_tag_in,
`endif
        output logic                  hit_out
    );
        return hit_out;
    endfunction

    protected task automatic update_entry
    (
        input  logic           [11:0] upd_idx,
`ifndef READ_ONLY_TLB
        input  tlb_way_t [1:0] [3:0]  upd_way,
`endif
        output logic                  upd_done
    );
        upd_done = 1'b1;
    endtask
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
