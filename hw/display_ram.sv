module display_ram(
  input logic clk,
  input logic [7:0] aa, ab,   // Addresses
  input logic [7:0] da, db,  // Data in
  input logic wa, wb,         // Write Enables
  output logic [7:0] qa, qb  // Data out
);

logic [7:0] mem [255:0];
always_ff @(posedge clk) begin
  if (wa) begin
    mem[aa] <= da;
    qa <= da;
  end else qa <= mem[aa];
end
always_ff @(posedge clk) begin
  if (wb) begin
    mem[ab] <= db;
    qb <= db;
  end else qb <= mem[ab];
end

endmodule

