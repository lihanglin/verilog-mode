module assign_align_01 (
    input  logic        clk,
    input  logic        rst_n,
    input  logic [15:0] a_i,
    input  logic [15:0] b_i,
    output logic [15:0] sum_o,
    output logic        valid_o
);
    logic [15:0] next_sum;
    logic        next_valid;
    logic [3:0]  ctrl_flags_d;

    always_comb begin
        // Combinational blocking assignments
          next_sum = a_i + b_i; // Add inputs
`ifdef FORCE_VALID
        next_valid = 1'b1;
`else
           next_valid = |a_i; // Non-zero check
`endif
         ctrl_flags_d = a_i[3:0] ^ b_i[3:0];
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
             sum_o <= 16'h0;
           valid_o <= 1'b0;
        end else begin
            // Mixed <= and = alignment
              sum_o <= next_sum;
          valid_o = next_valid;
        end
    end
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
// verilog-auto-lineup: all
// verilog-auto-newline: nil
// verilog-auto-lineup-on-semi: nil
// End:
