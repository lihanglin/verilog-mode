class scoreboard_types_c;

    typedef struct packed
    {
        bit              vld;
        bit       [15:0] addr;
        bit [3:0] [7:0]  mask;
    } req_info_t;

    typedef union packed
    {
        req_info_t        info;
        bit        [48:0] raw_bits;
    } req_entry_u;

endclass
// Local Variables:
// verilog-indent-level: 4
// verilog-indent-level-module: 4
// verilog-indent-level-declaration: 4
// verilog-cexp-indent: 4
// verilog-align-decl-dimension: t
// verilog-align-decl-sub-dimension: t
// indent-tabs-mode: nil
// End:
