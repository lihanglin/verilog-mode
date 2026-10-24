module mixed_types;
   wire                    [15:0] bus_wire;
   reg               [3:0] [15:0] bus_reg;
   logic       [1:0] [3:0] [15:0] bus_logic;
   byte                           status_byte;
   int                            counter_int;
   tlb_entry_t             [15:0] tlb_way;
endmodule
// Local Variables:
// verilog-align-decl-dimension: t
// verilog-align-decl-sub-dimension: t
// End:
