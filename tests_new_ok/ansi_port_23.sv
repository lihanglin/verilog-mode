module ansi_port_23;
    function automatic logic func_calc_hazard
    (
        // Read pointer input
        input  logic          [IfuPtrW-1:0] read_ptr,
`ifdef HAZARD_EXT
        /* Extended hazard check inputs
         spanning two lines */
        input  hazard_t [3:0] [IfuPtrW-1:0] ext_hazard_vec,
`else
        input  logic          [IfuPtrW-1:0] base_hazard_vec,
`endif
        output logic          [1:0]         hazard_vld_o
    );
        return |hazard_vld_o;
    endfunction
endmodule
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
