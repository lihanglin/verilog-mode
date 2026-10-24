module pkt_parser;

    typedef union packed
    {
        logic [3:0] [15:0] half_words;
        logic [7:0] [7:0]  bytes;
        logic       [63:0] raw_qword;
    } pkt_view_u;

endmodule
// Local Variables:
// verilog-indent-level: 4
// verilog-indent-level-module: 4
// verilog-indent-level-declaration: 4
// verilog-cexp-indent: 4
// verilog-align-decl-dimension: t
// verilog-align-decl-sub-dimension: t
// indent-tabs-mode: nil
// End:
