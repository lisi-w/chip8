module cpu(input logic clk,
  input logic [15:0] keystate,
  input logic reset,

  output logic disp_ram_req,  // If we're asking for rw op on display ram.

  output logic [7:0] disp_aa, disp_ab, // Addresses
  output logic [7:0] disp_da, disp_db, // Data in
  output logic disp_wa, disp_wb,       // Write Enables
  input logic [7:0] disp_qa, disp_qb,  // Data out

  output logic [11:0] ch_aa, ch_ab,     // Addresses
  output logic [7:0] ch_da, ch_db,     // Data in
  output logic ch_wa, ch_wb,           // Write Enables
  input logic [7:0] ch_qa, ch_qb       // Data out

);

`define PROG_START 16'h200;

enum {FETCHING, EXECUTING} state;

// Every variable with a _next counterpart stores the value in the
// register. the _next counterpart combinationally determines
// the value the register will be assigned on the next posedge clk.

logic [15:0] pc /* register */, pc_next /* wire */;
// Stack pointer points to location where next address will be written.
logic [3:0] sp /* register */, sp_next /* wire */;
logic [15:0] stack[16];
logic [7:0] regs[8];
logic [15:0] opcode;  // Currently executing opcode.
logic do_opcode_capture_next_cycle;

initial begin
  state = FETCHING;
  sp = 0;
  pc = `PROG_START;
end

always_comb begin
  /* Fill me in! */
  pc_next = pc;
  sp = sp_next;

end


always_ff @(posedge clk) begin
  if (reset) begin
    sp <= 0;
  end
end


endmodule
