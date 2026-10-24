module foo;

    function automatic logic func_sras_has_entry
               (
        logic [IfuSrasPtrWidth-1:0] sras_read_ptr,
        logic [IfuSrasPtrWidth-1:0] sras_retire_ptr,
        logic [1:0] [IfuSrasPtrWidth-1:0] sras_write_ptr
               );

        func_sras_has_entry   = func_sras_ptr_in_vld_range(sras_read_ptr, sras_retire_ptr, sras_write_ptr);
    endfunction

    task automatic drive_lanes
          (
        input logic clk,
        input logic [3:0] [7:0] lane_in,
        output logic [7:0] lane_out
          );
        lane_out = lane_in[0];
    endtask

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
