typedef union packed {
    logic [63:0] raw_qword;
    logic [1:0][31:0] dwords;
    idu_instr_pkt_t instr_pkt;
} pipe_payload_u;
// Local Variables:
// verilog-align-decl-dimension: t
// verilog-align-decl-sub-dimension: nil
// End:
