module struct_union_16;
    typedef struct packed
    {
        // Header valid bit
        logic                            valid;
`ifdef ZRF_EXT_FIELDS
        /* Extended xuop and
         register fields */
        idu_opcode_e [1:0] [10:0]        opcode;
        bit                [WXuopId-1:0] xuopid;
`elsif ZRF_COMPACT_FIELDS
        bit                [7:0]         compact_id;
`else
        bit                [3:0]         base_id;
`endif
        // Final destination field
        bit                [WZLrnId-1:0] zlrn;
    } zrf_wr_info_t;
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
