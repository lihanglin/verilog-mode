class ansi_port_26_c;
    function new
          (
    // Constructor name
       string name = "ansi_port_26_c",
`ifdef UVM_COMP_ARGS
     /* Parent component and
        configuration mask */
   uvm_component parent = null,
        logic [1:0] [7:0] cfg_mask = 16'hffff,
`else
  int unsigned inst_id = 0,
`endif
      bit [3:0] debug_lvl = 4'h0
         );
    endfunction
endclass
// Local Variables:
// verilog-indent-lists: nil
// verilog-indent-level: 4
// verilog-indent-level-module: 4
// verilog-indent-level-declaration: 4
// verilog-indent-level-behavioral: 4
// verilog-cexp-indent: 4
// verilog-case-indent: 4
// indent-tabs-mode: nil
// verilog-align-decl-dimension: t
// verilog-align-decl-sub-dimension: t
// verilog-auto-newline: nil
// verilog-auto-lineup-on-semi: nil
// End:
