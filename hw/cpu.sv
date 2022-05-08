module cpu(input logic clk,
  input logic may_run,

  output logic ram_req,  // If we're asking for rw op on any ram.

  output logic [7:0] disp_aa, disp_ab, // Addresses
  output logic [7:0] disp_da, disp_db, // Data in
  output logic disp_wa, disp_wb,       // Write Enables
  input logic [7:0] disp_qa, disp_qb,  // Data out

  output logic [7:0] ch_aa, ch_ab,     // Addresses
  output logic [7:0] ch_da, ch_db,     // Data in
  output logic ch_wa, ch_wb,           // Write Enables
  input logic [7:0] ch_qa, ch_qb,      // Data out

  input logic [15:0] keystate
);


always_ff @(posedge clk) begin
  if (may_run) begin
  end
end


endmodule
