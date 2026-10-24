typedef struct packed {
    logic [7:0] cmd;
    addr_t [31:0] addr;
} req_hdr_t;

typedef union packed {
    req_hdr_t hdr;
    logic [39:0] raw;
} req_pkt_u;
// Local Variables:
// verilog-align-decl-dimension: t
// End:
