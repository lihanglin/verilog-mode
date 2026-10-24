module foo;

    //===============================Function ==================================================//
    function automatic logic fn_check_thermal (
        input [3:0] in
    );
    logic [3:0] in_p1;

        in_p1 = in + 1'b1;
        // Returns 1 if valid thermal code, 0 otherwise
        return (in & in_p1) == 4'b0000;
    endfunction


    function automatic logic func_sras_has_entry (
        logic [IfuSrasPtrWidth-1:0]       sras_read_ptr,
        logic [IfuSrasPtrWidth-1:0]       sras_retire_ptr,
	logic [1:0] [IfuSrasPtrWidth-1:0] sras_write_ptr
    );

        func_sras_has_entry   = func_sras_ptr_in_vld_range(sras_read_ptr, sras_retire_ptr, sras_write_ptr);
    endfunction

    function automatic logic func_nras_has_entry (logic [IfuNrasPtrWidth-1:0] nras_read_ptr, logic [IfuNrasPtrWrapBits-1:0] nras_read_data_tag);
        func_nras_has_entry   = (nras_read_ptr[(IfuNrasPtrWidth-1)-:IfuNrasPtrWrapBits] == nras_read_data_tag);
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
