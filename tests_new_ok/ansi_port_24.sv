module ansi_port_24;
    task automatic drive_packet
    (
        input  logic           [3:0]  chan_id,
        input  pkt_hdr_t [1:0] [15:0] hdr_in,
`ifdef DRIVE_DMA
        /* DMA channel output */
        output logic     [3:0] [63:0] dma_payload_o
`elsif DRIVE_PIO
        output logic           [31:0] pio_payload_o
`else
        // Fallback status output
        output logic                  status_o
`endif
    );
        status_o = 1'b1;
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
