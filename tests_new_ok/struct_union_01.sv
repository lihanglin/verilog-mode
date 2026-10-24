typedef struct packed {
    logic        [3:0]  tag;
    idu_opcode_e [10:0] opcode;
    logic               valid;
} idu_instr_pkt_t;
// Local Variables:
// verilog-align-decl-dimension: t
// End:
