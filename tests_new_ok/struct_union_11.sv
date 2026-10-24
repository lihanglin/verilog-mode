class smu_idu_scoreboard_c extends uvm_scoreboard;

    `uvm_component_utils(smu_idu_scoreboard_c)

    typedef struct packed
    {
        bit               valid;
        bit [WXuopId-1:0] xuopid;
        bit [WZLrnId-1:0] zlrn;
    } zrf_wr_info_t;

    // Reset event
    uvm_event m_reset_event;

endclass
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
