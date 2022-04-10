

// VGA????????640*480@60

module vga_driver(
	input	wire 			clk_25MHz,	//clk_frq for different VGA resolutions
	input	wire				rst,
	input	wire		[15:0]	pixel_data,	//RGB--565,pixel_data[15:11]control_R;pixel_data[10:5]control_G;pixel_data[4:0]control_B
	
	output	wire		[  9:0]	pixel_hpos,	//pixel_data-h ordinate
	output	wire		[  9:0]	pixel_vpos,	//pixel_data-v ordinate
	output	wire					vga_hs,		//h-syn signal
	output	wire					vga_vs,		//h-syn signal 
	output	wire		[15:0]	vga_rgb		// output color to VGA port
);

//define internal parameter ,vga timing parameter
parameter           H_SYNC  	= 10'd96;	
parameter           H_BACK  	= 10'd48;	
parameter           H_DISP  	= 10'd640;    	
parameter           H_FRONT 	= 10'd16;	
parameter           H_PRIOD 	= 10'd800;  	
 
parameter           V_SYNC  	= 10'd2;		
parameter           V_BACK  	= 10'd33;	
parameter           V_DISP  	= 10'd480;     
parameter           V_FRONT 	= 10'd10;	
parameter           V_PRIOD	= 10'd525;	


// enable signal
wire 	vga_en;
// require signal 
wire 	pixel_data_require;


reg 		[9:0]	cnt_h;
reg 		[9:0]	cnt_v;


assign 	vga_hs = (cnt_h <= H_SYNC)	? 1'b0 : 1'b1;
assign 	vga_vs = (cnt_v <=  V_SYNC)	? 1'b0 : 1'b1;

// find effective area
assign 	vga_en = (((cnt_h >= H_SYNC + H_BACK) && (cnt_h < H_SYNC + H_BACK + H_DISP)) 
				&& ((cnt_v >= V_SYNC + V_BACK) &&(cnt_v < V_SYNC + V_BACK + V_DISP)))
				? 1'b1 : 1'b0;

// re_siganl one cycle before data
assign pixel_data_require = (((cnt_h >= H_SYNC + H_BACK - 1'b1) && (cnt_h <  H_SYNC + H_BACK + H_DISP - 1'b1)) &&    
                  ((cnt_v>= V_SYNC + V_BACK) && (cnt_v <  V_SYNC + V_BACK + V_DISP))) ? 1'b1 : 1'b0; 
 
// find pixel coordinate
assign	pixel_hpos = pixel_data_require ? (cnt_h - (H_SYNC + H_BACK - 1'b1)) : 10'd0;
assign	pixel_vpos = pixel_data_require ? (cnt_v - (V_SYNC + V_BACK - 1'b1)) : 10'd0;

// find pixeldata
assign vga_rgb = vga_en ? pixel_data:16'd0;

// count 
always @ (posedge clk_25MHz or negedge rst) begin
	if (!rst) begin
		cnt_h <= 10'd0;
	end
	else begin
		if (cnt_h <= H_PRIOD - 1'b1) begin
			cnt_h <= cnt_h + 1'b1;
		end
		else begin
			cnt_h <= 10'd0;
		end
	end
end

always @ (posedge clk_25MHz or negedge rst) begin
	if(!rst)begin
		cnt_v <= 10'd0;
	end
	else begin
		if (cnt_h == H_PRIOD - 1'b1) begin
			if(cnt_v <= V_PRIOD - 1'b1)
				cnt_v <= cnt_v + 1'b1;
			else begin
				cnt_v <= 10'd0;
			end
		end
	end
end

endmodule