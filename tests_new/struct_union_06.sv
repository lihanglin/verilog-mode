typedef struct packed {
    logic [3:0] [7:0] byte_lanes;
    idu_uop_t [7:0] uops;
    logic [1:0][3:0][7:0] matrix;
    logic parity;
} matrix_pkt_t;
// Local Variables:
// verilog-align-decl-dimension: t
// verilog-align-decl-sub-dimension: t
// End:
