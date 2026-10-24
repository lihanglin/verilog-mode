module task_ports_test;
    task automatic drive_burst (
        input  logic       [7:0]  beat_cnt = 8'd1,
        output logic [3:0] [31:0] burst_data,
        ref    logic       [31:0] mem_view [0:15]
    );
        burst_data = '0;
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
// verilog-align-decl-sub-dimension: t
// verilog-auto-newline: nil
// verilog-auto-lineup-on-semi: nil
// End:
