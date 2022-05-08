module display(input logic clk,
  // We ask for the byte at some address on the screen.
  output logic [7:0] addr,
  input logic [7:0] data,

  output logic [7:0] VGA_R, VGA_G, VGA_B,
  output logic 	   VGA_CLK, VGA_HS, VGA_VS, VGA_BLANK_n,
  output logic 	   VGA_SYNC_n);


endmodule
