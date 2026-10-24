module func_combined_dim;
    function automatic logic func_sras_combined (
        logic [IfuSrasPtrWidth-1:0] sras_read_ptr,
    custom_ptr_t [IfuSrasPtrWidth-1:0] sras_retire_ptr,
	logic [1:0] [IfuSrasPtrWidth-1:0] sras_write_ptr
    );
        return 1'b1;
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
// verilog-align-decl-sub-dimension: nil
// verilog-auto-newline: nil
// verilog-auto-lineup-on-semi: nil
// End:
