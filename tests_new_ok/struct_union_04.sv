package noc_pkg;
   typedef struct packed {
      common_pkg::node_id_t         src_id;
      common_pkg::node_id_t [1:0]   dst_id;
      logic                 [127:0] payload;
      logic                         eop;
   } noc_flit_t;
endpackage
// Local Variables:
// verilog-align-decl-dimension: t
// End:
