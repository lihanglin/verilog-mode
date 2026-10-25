class decl_dim_21_c;
    virtual function automatic logic eval_score
    (
        input logic             [7:0]  idx_in,
        input score_pkt_t [1:0] [15:0] pkt_in
    );
        // Local declarations inside class virtual function
        logic        [3:0] [15:0] lane_score;
`ifdef SCORE_WEIGHTED
        /* Weighted evaluation temporaries */
        weight_cfg_t [1:0] [7:0]  lane_weight;
`elsif SCORE_SCALED
        logic              [7:0]  lane_scale;
`else
        logic                     lane_bypass;
`endif
        logic                     match_flag;

        match_flag = |lane_score;
        return match_flag;
    endfunction
endclass
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
