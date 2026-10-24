module csr_unit;
   struct packed {
      logic [31:0] mstatus;
      csr_priv_e [1:0] priv_mode;
      logic irq_en;
   } csr_state_q;
endmodule
// Local Variables:
// verilog-align-decl-dimension: t
// End:
