module task_and_seq_block;
    task automatic run_xact (
        input logic clk
    );
        logic        [7:0][31:0] task_buf;
        custom_req_t [31:0]      task_req;

        begin : inner_blk
            logic       [1:0][15:0] blk_vec;
            short_hdr_t [15:0]      blk_hdr;
            blk_vec = '0;
        end
    endtask
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
