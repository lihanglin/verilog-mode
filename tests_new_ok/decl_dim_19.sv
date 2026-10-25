module decl_dim_19;
    function automatic logic [31:0] compute_crc
    (
        input logic [31:0] data_in,
        input logic [7:0]  poly_in
    );
        // Local variables inside function with directives and comments
        logic     [3:0] [7:0]  byte_lanes;
`ifdef CRC_FAST_TABLE
        /* Lookup table temporary
         variables */
        crc_tbl_t [1:0] [15:0] tbl_val;
`else
        logic           [15:0] shift_reg;
`endif
        // Final accumulator
        logic           [31:0] crc_acc;

        crc_acc = data_in ^ {24'h0, poly_in};
        return crc_acc;
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
