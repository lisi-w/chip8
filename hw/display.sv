module display(input logic clk,
  // We ask for the byte at some address on the screen.
  output logic [7:0] addr,
  input logic [7:0] data,

  output logic [7:0] VGA_R, VGA_G, VGA_B,
  output logic 	   VGA_CLK, VGA_HS, VGA_VS, VGA_BLANK_n,
  output logic 	   VGA_SYNC_n);

   logic [10:0]	   hcount;
   logic [9:0]     vcount;  
	
   fb_counters counters(.clk50(clk), .*);
   parameter DISP_COLS = 10'd640;
   parameter DISP_ROWS = 10'd480;
   parameter DISP_COL_OFFSET = 10'd64;
   parameter DISP_ROW_OFFSET = 10'd112;
   parameter CHIP8_SCALING_FACTOR = 8;

   logic [9:0] disp_col;  // hcount[10:1] is pixel column
   logic [9:0] disp_row;  // vcount[9:0] is pixel row
   logic chip8_area;
   logic is_foreground;
   logic [5:0] chip8_col;
   logic [4:0] chip8_row;
   logic [7:0] d_is_foreground;
   logic [7:0] mac
   assign mac = 8'h80 >> (chip8_col % 8);

   assign disp_col = hcount[10:1];
   assign disp_row = vcount;

   assign chip8_col = (disp_col - DISP_COL_OFFSET) / CHIP8_SCALING_FACTOR;
   assign chip8_row = (disp_row - DISP_ROW_OFFSET) / CHIP8_SCALING_FACTOR;
   assign chip8_area = (disp_col >= DISP_COL_OFFSET) && (disp_col < (DISP_COLS - DISP_COL_OFFSET)) 
                          && (disp_row >= DISP_ROW_OFFSET) && (disp_row < (DISP_ROWS - DISP_ROW_OFFSET));
   assign addr = (chip8_row * 8'h08) + ( chip8_row / 8);
   assign d_is_foreground = data & mac;
   assign is_foreground = d_is_foreground > 0 ? 1 : 0;

	always_comb begin 
		{VGA_R,VGA_G,VGA_B}={8'h00,8'h00,8'h00};
		if ((VGA_BLANK_n) && (chip8_area) && (is_foreground))
		begin
			{VGA_R,VGA_G,VGA_B} = {8'hff, 8'hff, 8'hff};
		end
	else begin
		{VGA_R, VGA_G, VGA_B} = {8'hff, 8'h00, 8'h00};
	end
end

endmodule

module fb_counters(
 input logic 	     clk50, reset,
 output logic [10:0] hcount,  // hcount[10:1] is pixel column
 output logic [9:0]  vcount,  // vcount[9:0] is pixel row
 output logic 	     VGA_CLK, VGA_HS, VGA_VS, VGA_BLANK_n, VGA_SYNC_n);

/*
 * 640 X 480 VGA timing for a 50 MHz clock: one pixel every other cycle
 * 
 * HCOUNT 1599 0             1279       1599 0
 *             _______________              ________
 * ___________|    Video      |____________|  Video
 * 
 * 
 * |SYNC| BP |<-- HACTIVE -->|FP|SYNC| BP |<-- HACTIVE
 *       _______________________      _____________
 * |____|       VGA_HS          |____|
 */
   // Parameters for hcount
   parameter HACTIVE      = 11'd 1280,
             HFRONT_PORCH = 11'd 32,
             HSYNC        = 11'd 192,
             HBACK_PORCH  = 11'd 96,   
             HTOTAL       = HACTIVE + HFRONT_PORCH + HSYNC +
                            HBACK_PORCH; // 1600
   
   // Parameters for vcount
   parameter VACTIVE      = 10'd 480,
             VFRONT_PORCH = 10'd 10,
             VSYNC        = 10'd 2,
             VBACK_PORCH  = 10'd 33,
             VTOTAL       = VACTIVE + VFRONT_PORCH + VSYNC +
                            VBACK_PORCH; // 525

   logic endOfLine;
   
   always_ff @(posedge clk50 or posedge reset)
     if (reset)          hcount <= 0;
     else if (endOfLine) hcount <= 0;
     else  	         hcount <= hcount + 11'd 1;

   assign endOfLine = hcount == HTOTAL - 1;
       
   logic endOfField;
   
   always_ff @(posedge clk50 or posedge reset)
     if (reset)          vcount <= 0;
     else if (endOfLine)
       if (endOfField)   vcount <= 0;
       else              vcount <= vcount + 10'd 1;

   assign endOfField = vcount == VTOTAL - 1;

   // Horizontal sync: from 0x520 to 0x5DF (0x57F)
   // 101 0010 0000 to 101 1101 1111
   assign VGA_HS = !( (hcount[10:8] == 3'b101) &
		      !(hcount[7:5] == 3'b111));
   assign VGA_VS = !( vcount[9:1] == (VACTIVE + VFRONT_PORCH) / 2);

   assign VGA_SYNC_n = 1'b0; // For putting sync on the green signal; unused
   
   // Horizontal active: 0 to 1279     Vertical active: 0 to 479
   // 101 0000 0000  1280	       01 1110 0000  480
   // 110 0011 1111  1599	       10 0000 1100  524
   assign VGA_BLANK_n = !( hcount[10] & (hcount[9] | hcount[8]) ) &
			!( vcount[9] | (vcount[8:5] == 4'b1111) );

   /* VGA_CLK is 25 MHz
    *             __    __    __
    * clk50    __|  |__|  |__|
    *        
    *             _____       __
    * hcount[0]__|     |_____|
    */
   assign VGA_CLK = hcount[0]; // 25 MHz clock: rising edge sensitive
   
endmodule
