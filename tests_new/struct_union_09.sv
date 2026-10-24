typedef struct packed {
    logic [3:0] tag; // transaction tag
    idu_opcode_e [10:0] opcode; // decoded opcode
    logic valid; // entry valid bit
} commented_pkt_t;
// Local Variables:
// verilog-align-decl-dimension: t
// End:
