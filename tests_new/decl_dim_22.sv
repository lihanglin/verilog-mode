module decl_dim_22 #(
    parameter int N_BANKS = 4
);
    generate
        for (genvar g = 0; g < N_BANKS; g++) begin : gen_bank
          // Per-bank signals
             logic [3:0] [7:0] bank_tag;
`ifdef BANK_ECC_EN
          /* ECC syndrome and
             corrected data */
        ecc_syn_t [1:0] [7:0] bank_syn;
`endif
           logic [31:0] bank_data;
        end
    endgenerate

    always_comb begin : comb_arb
      // Arbiter local temporaries
         logic [3:0] arb_req;
`ifdef ARB_PRIO_MATRIX
        prio_mat_t [3:0] [3:0] arb_mat;
`else
     logic [3:0] arb_mask;
`endif
       logic arb_vld;
        arb_vld = |arb_req;
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
// verilog-auto-newline: nil
// verilog-auto-lineup-on-semi: nil
// End:
