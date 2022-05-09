module yachip8(input logic clk,
  input logic reset,
  input logic [15:0] writedata,
  //output logic [15:0] readdata,
  input logic 	   write,
  input 		   chipselect,
  input logic [11:0]  address,

  output logic [7:0] VGA_R, VGA_G, VGA_B,
  output logic 	   VGA_CLK, VGA_HS, VGA_VS, VGA_BLANK_n,
  output logic 	   VGA_SYNC_n);

// Miscellaneous variables.
logic [15:0] keystate;  // updated in "control logics" section.

// Note that the word "req" here is shorthand for "request".

//
// Instantiate rams.
//

logic [7:0] disp_aa, disp_ab;
logic [7:0] disp_da, disp_db;
logic       disp_wa, disp_wb;
logic [7:0] disp_qa, disp_qb;

display_ram disp_ram(.clk,
  .aa(disp_aa), .ab(disp_ab),
  .da(disp_da), .db(disp_db),
  .wa(disp_wa), .wb(disp_wb),
  .qa(disp_qa), .qb(disp_qb)
);

logic [11:0] ch_aa, ch_ab;
logic [7:0] ch_da, ch_db;
logic ch_wa, ch_wb;
logic [7:0] ch_qa, ch_qb;
chip_ram ch_ram(.clk,
  .aa(ch_aa), .ab(ch_ab),
  .da(ch_da), .db(ch_db),
  .wa(ch_wa), .wb(ch_wb),
  .qa(ch_qa), .qb(ch_qb)
);

//
// Instantiate cpu
//

logic cpu_may_run, cpu_disp_ram_req;
logic  [7:0] cpu_disp_aa_req, cpu_disp_ab_req;
logic  [7:0] cpu_disp_da_req, cpu_disp_db_req;
logic        cpu_disp_wa_req, cpu_disp_wb_req;
logic [11:0] cpu_ch_aa_req, cpu_ch_ab_req;
logic  [7:0] cpu_ch_da_req, cpu_ch_db_req;
logic        cpu_ch_wa_req, cpu_ch_wb_req;

cpu ch(.clk, .keystate,
  .may_run(cpu_may_run),

  .disp_ram_req(cpu_disp_ram_req),
  .disp_wa(cpu_disp_wa_req), .disp_wb(cpu_disp_wb_req),
  .disp_da(cpu_disp_da_req), .disp_db(cpu_disp_db_req),
  .disp_aa(cpu_disp_aa_req), .disp_ab(cpu_disp_ab_req),
  .disp_qa, .disp_qb,
  .ch_wa(cpu_ch_wa_req), .ch_wb(cpu_ch_wb_req),
  .ch_da(cpu_ch_da_req), .ch_db(cpu_ch_db_req),
  .ch_aa(cpu_ch_aa_req), .ch_ab(cpu_ch_ab_req),
  .ch_qa, .ch_qb
);

//
// Instantiate display
//

logic [7:0] display_addr_req;

display disp(.clk,
  .addr(display_addr_req), .data(disp_qa),
  .VGA_R, .VGA_G, .VGA_B, .VGA_CLK, .VGA_HS, .VGA_VS, .VGA_BLANK_n, .VGA_SYNC_n
);

//
// Control logics
//

// Logics to control who is touching what.
// The hierarchy is as follows:
// 1. the controller may write to any address.
// 2. Else, we let the cpu ask.
// 3. Third, we let the display ask.

// Characterize read/writes.
// (whether the requested address is the 4k ram, a keystate, etc).
logic req_addr_is_ch_ram, req_addr_is_disp_ram,
  req_addr_is_keystate, req_addr_is_control;
assign req_addr_is_ch_ram   = (address < 12'h800);
assign req_addr_is_disp_ram = (12'h800 <= address) && (address < 12'h880);
assign req_addr_is_keystate = (12'hFFE == address);
assign req_addr_is_control  = (12'hFFF == address);

logic [11:0] address_to_ch_ram_lower_addr, address_to_ch_ram_upper_addr;
assign address_to_ch_ram_lower_addr = address << 1;
assign address_to_ch_ram_upper_addr = address << 1 | 12'b1;
logic [7:0] address_to_disp_ram_lower_addr, address_to_disp_ram_upper_addr;
assign address_to_disp_ram_lower_addr = {1'b0, address[6:0]};
assign address_to_disp_ram_upper_addr = {1'b0, address[6:0]} << 1 | 8'b1;

// Delegate ram access to display, ram, or user of this module.
always_comb begin
  cpu_may_run = 0;
  {disp_aa, disp_ab, disp_da, disp_db, disp_wa, disp_wb} = 0;
  {ch_aa, ch_ab, ch_da, ch_db, ch_wa, ch_wb} = 0;

  if (write && req_addr_is_ch_ram) begin
    ch_aa = address_to_ch_ram_lower_addr;
    ch_ab = address_to_ch_ram_upper_addr;
    ch_da = writedata[15:8];
    ch_da = writedata[7:0];
    {ch_wa, ch_wb} = 2'b11;
  end else if (write && req_addr_is_disp_ram) begin
    disp_aa = address_to_disp_ram_lower_addr;
    disp_ab = address_to_disp_ram_upper_addr;
    disp_da = writedata[15:8];
    disp_da = writedata[7:0];
    {disp_wa, disp_wb} = 2'b11;
  end else begin
    // If the controller isn't writing, cpu can do what it wants.
    cpu_may_run = 1;
    // If the controller isn't writing, cpu controls ch_ram.
    {ch_aa, ch_ab} = {cpu_ch_aa_req, cpu_ch_ab_req};
    {ch_da, ch_db} = {cpu_ch_da_req, cpu_ch_db_req};
    {ch_wa, ch_wb} = {cpu_ch_wa_req, cpu_ch_wb_req};

    // display only gets disp_ram if cpu isn't asking for it.
    if (cpu_disp_ram_req) begin
      {disp_aa, disp_ab} = {cpu_disp_aa_req, cpu_disp_ab_req};
      {disp_da, disp_db} = {cpu_disp_da_req, cpu_disp_db_req};
      {disp_wa, disp_wb} = {cpu_disp_wa_req, cpu_disp_wb_req};
    end else begin
      disp_aa = display_addr_req;
    end
  end
end


initial keystate = 0;
always_ff @(posedge clk) begin
  if (write && req_addr_is_keystate) begin
    keystate <= writedata;
  end
end

endmodule
