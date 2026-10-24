// Sample SystemVerilog fixture for Phase 1 standalone tree-sitter tests
`define COUNTER_WIDTH 8
`define STEP_VAL(x) ((x) + 1)

/* Multi-line block comment
   containing keywords like module and endmodule
   that should be ignored by syntactic scanners. */

package sample_pkg;
   typedef enum logic [1:0] {
      IDLE = 2'b00,
      RUN  = 2'b01,
      STOP = 2'b10
   } state_e;
endpackage // sample_pkg

module sub_block #(
   parameter int WIDTH = 8
) (
   input  logic             clk,
   input  logic             rst_n,
   input  logic [WIDTH-1:0] in_data,
   output logic [WIDTH-1:0] out_data
);
   always_ff @(posedge clk or negedge rst_n) begin
      if (!rst_n) begin
         out_data <= '0;
      end
      else begin
         out_data <= in_data;
      end
   end
endmodule

module sample_counter #(
   parameter int WIDTH = `COUNTER_WIDTH
) (
   input  logic             clk,
   input  logic             rst_n,
   input  logic             enable,
   input  logic [WIDTH-1:0] load_val,
   output logic [WIDTH-1:0] count_out,
   output logic             overflow
);

   // Internal declarations
   logic [WIDTH-1:0] count_q;
   logic [WIDTH-1:0] count_d;
   logic [WIDTH-1:0] sub_out;
   string            banner = "module inside string // not a comment";

   // Combinational next-state logic
   always @* begin
      count_d = count_q;
      overflow = 1'b0;
      if (enable) begin
         {overflow, count_d} = count_q + load_val;
      end
   end

   // Sequential state register
   always_ff @(posedge clk or negedge rst_n) begin : seq_proc
      if (!rst_n) begin
         count_q <= '0;
      end
      else begin
         count_q <= count_d;
      end
   end : seq_proc

   // Sub-module instantiation
   sub_block #(
      .WIDTH (WIDTH)
   ) u_sub (
      .clk      (clk),
      .rst_n    (rst_n),
      .in_data  (count_q),
      .out_data (sub_out)
   );

   assign count_out = sub_out;

   function automatic logic [WIDTH-1:0] calc_next(input logic [WIDTH-1:0] cur);
      return cur + 1'b1;
   endfunction

   task automatic reset_task();
      count_q = '0;
   endtask

endmodule
