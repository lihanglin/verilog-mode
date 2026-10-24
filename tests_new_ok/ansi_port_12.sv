module func_dir_subdim;
    function automatic logic fn_arb_select (
        input  logic                            arb_en,
        input  idu_uop_t                  [3:0] uop_in,
        input  lsu_pkg::req_t       [1:0] [3:0] lsu_req_in,
        output logic          [7:0] [1:0] [3:0] grant_mask_out
    );
        return arb_en;
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
