typedef struct packed
   {
    logic [3:0] [7:0] byte_lanes;
    idu_uop_t [7:0] uops;
    logic [1:0] [3:0] [7:0] matrix;
    logic parity;
   } matrix_allman_pkt_t;
// Local Variables:
// verilog-indent-level: 4
// verilog-indent-level-declaration: 4
// verilog-cexp-indent: 4
// verilog-align-decl-dimension: t
// verilog-align-decl-sub-dimension: nil
// indent-tabs-mode: nil
// End:
