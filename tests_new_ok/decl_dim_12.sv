module func_multi_decl;
    function automatic logic [63:0] fn_pack_pkt (
        input logic [3:0] sel
    );
        logic           [3:0] [63:0] stage_vec;
        idu_pkt_t             [63:0] stage_pkt;
        logic     [1:0] [3:0] [63:0] pipe_vec;
        logic                        valid_bit;

        stage_vec = '0;
        return stage_pkt[sel];
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
