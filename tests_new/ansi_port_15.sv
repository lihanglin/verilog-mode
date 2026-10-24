module cls_method_ports;
    class mmu_checker;
        virtual function automatic logic check_tlb (
        input logic [1:0] [11:0] vpn_idx,
    input tlb_tag_t [11:0] tag_in,
	output logic hit_out
  );
            return hit_out;
        endfunction
    endclass
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
