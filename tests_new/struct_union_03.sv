typedef struct packed {
    bit vld;
    bit [1:0] lane_mask;
    shortint offset;
    logic [3:0] words;
    longint timestamp;
} trace_entry_t;
// Local Variables:
// verilog-align-decl-dimension: t
// End:
