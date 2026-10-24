class uvm_agent_allman_c;

    function new
       (
        string name = "uvm_agent_allman_c",
        bit [7:0] agent_id = 8'h0
       );
    endfunction

    virtual function automatic void write_pkt
       (
        input bit vld,
        input bit [3:0] [15:0] pkt_data
       );
    endfunction

    extern virtual function automatic bit check_status
       (
        input bit [15:0] status_mask,
        input bit [1:0] [15:0] status_vec
       );

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
