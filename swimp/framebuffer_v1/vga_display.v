
// display module 
// save image, read data for driver

module vga_display(
	input 	wire		clk_25MHz,
	input	wire		rst,
	input	wire		[9:0]	pixel_hpos,
	input	wire		[9:0]	pixel_vpos,
	
	output	reg		[15:0]	pixel_data
);

parameter H_DISP =10'd640;		// h_resolution
parameter V_DISP =10'd480;		// v_resolution 
 
localparam WHITE = 16'b11111_111111_11111;
localparam BLACK = 16'b00000_000000_00000;
localparam RED   = 16'b11111_000000_00000;
localparam GREEN = 16'b00000_111111_00000;
localparam BLUE  = 16'b00000_000000_11111;

parameter [9:0] PIC_HPOS = 10'd0;	
parameter [9:0] PIC_VPOS = 10'd0;	
parameter [9:0] PIC_WIDTH = 10'd75;	// width 
parameter [9:0] PIC_HEIGHT = 10'd105;	// height

// current coordinate vs. display coordinate
reg pic_area;		
always @ (posedge clk_25MHz or negedge rst) begin
	if(!rst)begin
		pic_area = 1'b0;
	end
	else begin
		if((pixel_hpos >= PIC_HPOS) && (pixel_hpos < PIC_HPOS + PIC_WIDTH) && (pixel_vpos >= PIC_VPOS) && (pixel_vpos < PIC_VPOS + PIC_HEIGHT))begin
			pic_area <= 1'b1;
		end
		else begin
			pic_area <= 1'b0;
		end
	end
end

// RAM
reg [13:0]wraddr;
reg [12:0]rdaddr;
reg rden;
reg wren;
reg [7:0]wrdata;
wire [15:0]rddata;

RAM RAM1(
		.data(wrdata),
		.rdaddress(rdaddr),
		.rdclock(clk_25MHz),
		.rden(rden),
		.wraddress(wraddr),
		.wrclock(clk_25MHz),
		.wren(wren),
		.q(rddata)
	);
	
// RAM rden
always@(posedge clk_25MHz or negedge rst)begin
	if(!rst)begin
		rden <= 1'b0;
	end
	else begin
		rden <= 1'b1;
	end
end

// RAM rdaddr 
always@(posedge clk_25MHz or negedge rst)begin
	if(!rst)begin
		rdaddr <= 13'd0;
	end
	else begin
		if(pic_area == 1'b1)begin
			rdaddr <=(pixel_hpos - PIC_HPOS) + ((pixel_vpos == 10'd0)?16'd0:((pixel_vpos - 1'b1)* PIC_WIDTH));
		end
	end
end

// RAM wren wraddr
always@(*)begin
	wraddr <= 14'd0;
	wren <= 1'b0;
	wrdata <= 8'd0;
end

// current pixel data
always @ (posedge clk_25MHz or negedge rst)begin
	if(!rst)begin
		pixel_data <= 16'd0;
	end
	else begin
		if(pic_area == 1'b0)begin
			if (pixel_hpos >= 0 && pixel_hpos <= (H_DISP / 5) * 1)
				pixel_data <= WHITE;
			else if (pixel_hpos >= (H_DISP / 5) * 1 && pixel_hpos < (H_DISP / 5) * 2)
				pixel_data <= BLACK;
			else if (pixel_hpos >= (H_DISP / 5) * 2 && pixel_hpos < (H_DISP / 5) * 3)
				pixel_data <= RED;
			else if (pixel_hpos >=(H_DISP / 5) * 3 && pixel_hpos < (H_DISP / 5) * 4)
				pixel_data <= GREEN;
			else
				pixel_data <= BLUE;
		end
		else begin
			pixel_data <= rddata;
		end
	end
end


endmodule