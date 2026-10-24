module comb_and_gen_scope #(
    parameter bit USE_FAST = 1'b1
);
    always_comb begin : comb_proc
    logic [3:0] [7:0] mux_data;
  alu_op_e [7:0] mux_op;
        mux_data = '0;
    end

    generate
        if (USE_FAST) begin : gen_fast
    logic [1:0] [31:0] fast_pipe;
        fast_pkt_t [31:0] fast_pkt;
        end
    endgenerate
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
