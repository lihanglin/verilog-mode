module sub_scope_struct;

    function automatic logic [31:0] pack_entry (
        input logic        vld,
        input logic [30:0] payload
    );
        typedef struct packed
        {
            logic        valid;
            logic [30:0] data;
        } local_entry_t;

        local_entry_t entry;
        entry.valid = vld;
        entry.data  = payload;
        return entry;
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
