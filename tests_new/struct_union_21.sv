module struct_union_21;
    function automatic logic check_struct_var
    (
        input logic [15:0] raw_in
    );
        struct packed
             {
          // Inline struct variable fields
             logic [1:0] [3:0] nibbles;
`ifdef INLINE_PARITY
          /* Parity bit inside
             function local struct */
        bit [1:0] par;
`endif
           logic [7:0] low_byte;
           } local_s;

        local_s = raw_in;
        return |local_s.low_byte;
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
