//VGA colorbar
//define port   
module vga_colorbar_top(
       input	wire				sys_clk ,
       input	wire				sys_rst,
       
       output	wire				vga_hs,			// ???vga????????
       output	wire				vga_vs,			// ???vga????????
       output	wire	[15:0]	vga_rgb			// ???vga???????
);

 
//define internal signal 
wire clk_w ;
wire locked_w;
 
wire [15:0] pixel_data_w;
wire [9:0]  pixel_hpos_w;
wire [9:0]  pixel_vpos_w;
 
//wire sys_rst_n;
 
 
//assign sys_rst_n = sys_rst && locked_w;
 
//pll    
PLL_48MHz_to_25MHz pll_25MHz(
    //.areset (sys_rst),
	.outclk_0 (sys_clk),
    
	.refclk     (clk_w)
	//.locked (locked_w)
 );   
        
//vga_driver
vga_driver u_vga_driver(
 //pll&rst
    .clk_25MHz      (clk_w),
    .rst      (sys_rst),
 //driver_output_signal   
    .vga_hs       (vga_hs),
    .vga_vs       (vga_vs),
    .vga_rgb      (vga_rgb),
//input pixel coordinate data signal in display module    
    .pixel_hpos   (pixel_hpos_w),
    .pixel_vpos   (pixel_vpos_w),
    .pixel_data   (pixel_data_w)
 );   
    
 //vga display module  
vga_display u_vga_disp(
    .clk_25MHz     (clk_w),
    .rst     (sys_rst), 
    
    .pixel_hpos   (pixel_hpos_w),
    .pixel_vpos   (pixel_vpos_w),   
    .pixel_data   (pixel_data_w)
);       
endmodule

