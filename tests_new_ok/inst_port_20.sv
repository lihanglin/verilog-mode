module inst_port_20
(
    input  logic              clk,
    input  logic              rst_n,
`ifdef TOP_WIDE_IN
    /* Wide top-level input bus */
    input  logic [3:0] [15:0] wide_in,
`else
    input  logic       [15:0] narrow_in,
`endif
    output logic       [31:0] top_out
);

    typedef struct packed
    {
        logic              valid;
`ifdef TOP_WIDE_IN
        logic [3:0] [15:0] payload;
`else
        logic       [15:0] payload;
`endif
    } top_item_t;

    // Internal declarations
    logic      [3:0] [15:0] stage_data;
`ifdef TOP_WIDE_IN
    top_item_t [1:0] [7:0]  stage_item;
`endif
    logic                   stage_vld;

    function automatic logic [31:0] fn_pack
    (
        input logic       [15:0] lo_word,
`ifdef TOP_WIDE_IN
        input logic [1:0] [15:0] hi_words
`else
        input logic       [15:0] hi_word
`endif
    );
        logic [3:0] [7:0]  local_bytes;
`ifdef TOP_WIDE_IN
        logic       [31:0] local_wide;
`endif
        logic       [15:0] local_lo;
        local_lo = lo_word;
        return {16'h0, local_lo};
    endfunction

    sub_engine u_engine
    (
        .clk            (clk),
        .rst_n          (rst_n),
`ifdef TOP_WIDE_IN
        .stage_data_i   (stage_data),
`else
        .stage_narrow_i (stage_data[0]),
`endif
        .engine_out_o   (top_out)
    );
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
