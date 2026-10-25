package struct_union_18_pkg;
    typedef union packed
    {
        // Raw vector view
        logic       [1:0] [31:0] raw_qword;
`ifndef DISABLE_PKT_VIEW
        /* Structured packet view
         inside union */
        pkt_view_t  [3:0] [15:0] pkt_half;
`else
        logic             [63:0] flat_bits;
`endif
        byte_lane_t [7:0] [7:0]  byte_lanes;
    } bus_payload_u;
endpackage
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
