
`timescale 1ns/1ps
module top_tb ();

reg clk_48MHz;
reg rst;
   wire vga_hs; 
   wire vga_vs; 
   wire [15:0] vga_rgb; 

initial begin
clk_48MHz = 1'b0;
forever #10.4 clk_48MHz = ~clk_48MHz;
end

initial begin
rst = 1'b0;
#50 rst = 1'b1;
//#1000 $stop;
end

vga_colorbar_top vga_colorbar_top1(
.sys_clk(clk_48MHz),
.sys_rst(rst),
.vga_hs(vga_hs),
.vga_vs(vga_vs),
.vga_rgb(vga_rgb)
);
endmodule