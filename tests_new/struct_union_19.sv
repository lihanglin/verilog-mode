interface struct_union_19_if;
    typedef struct packed
         {
       logic [`TAG_W-1:0] tag;
`ifdef IF_MACRO_FIELD
      /* Macro field type inside
         interface struct */
   `FIELD_T [3:0] [`DATA_W-1:0] data_lanes;
`else
        logic [1:0] [`DATA_W-1:0] fallback_lanes;
`endif
     logic valid; // trailing valid comment
       } if_req_pkt_t;
endinterface
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
