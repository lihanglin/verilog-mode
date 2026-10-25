module foo;

    generate
        for (genvar g_uop = 0; g_uop < NUopDe0; g_uop++) begin : gen_uop_dec
            gcpu_smu_idu_dec
                #(
`include "gcpu_smu_idu_subm_inst.svh"
`include "gcpu_inst_end.svh"
            )
            uop_dec_de1 (
                // Inputs
                .idu_fe_rclk(idu_fe_rclk),
                .idu__uclk(idu__uclk),
                .idu__rst(idu__rst),
                .idu__icg_force_on_en(idu__icg_force_on_en),
                .svd_uop(svd_uop_de1[g_uop]),
     // Outputs
                .xuop_vld(xuop_dec_result_vld_de1[g_uop]),
                .last_xuop_taken_de1(last_xuop_taken_de1[g_uop])
            );
        end
    endgenerate

endmodule
// Local Variables:
// verilog-indent-lists: nil
// verilog-indent-level: 4
// verilog-indent-level-module: 4
// verilog-indent-level-declaration: 4
// verilog-indent-level-behavioral: 4
// verilog-indent-level-directive: 0
// verilog-cexp-indent: 4
// verilog-align-decl-dimension: t
// verilog-align-decl-sub-dimension: t
// indent-tabs-mode: nil
// End:
