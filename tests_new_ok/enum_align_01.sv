package enum_align_01_pkg;
    typedef enum logic [2:0]
    {
        ST_RESET,
        ST_IDLE         = 3'd0, // Idle state
        ST_FETCH        = 3'd1, // Instruction fetch
        ST_DECODE_LONG,
        ST_EXEC         = 3'd4  // Execute stage
    } pipe_state_e;
endpackage
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
