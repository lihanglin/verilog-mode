module multiline_and_dpi;

    import "DPI-C" function int dpi_compute
       (
        input int mode,
        input bit [31:0] payload
       );

    function automatic logic [31:0]
        func_multiline_hdr
       (
        input logic [7:0] a,
        input logic [1:0] [7:0] b
       );
        return {24'h0, a};
    endfunction

endmodule
// Local Variables:
// verilog-indent-level: 4
// verilog-indent-level-module: 4
// verilog-indent-level-declaration: 4
// verilog-indent-level-behavioral: 4
// verilog-cexp-indent: 4
// verilog-align-decl-dimension: t
// verilog-align-decl-sub-dimension: t
// indent-tabs-mode: nil
// End:
