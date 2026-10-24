module cls_scope_test;
    class packet_xactor;
        logic      [3:0] [15:0] prop_vec;
        pkt_meta_t       [15:0] prop_meta;

        function void build_pkt();
            logic     [1:0] [7:0] local_arr;
            hdr_tag_t       [7:0] local_tag;
            logic                 local_vld;
            local_vld = 1'b1;
        endfunction
    endclass
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
