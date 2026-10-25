module struct_union_20;
`ifdef USE_WIDE_HDR
    typedef struct packed {
       logic [3:0] [15:0] wide_tag;
`ifdef HDR_EXTRA_META
      /* Extra metadata in wide header */
   meta_info_t [7:0] meta;
`endif
     logic vld;
    } hdr_pkt_t;
`else
    typedef union packed {
        logic [31:0] raw_word;
   hdr_pair_t [1:0] [15:0] pair_word;
    } hdr_pkt_t;
`endif
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
