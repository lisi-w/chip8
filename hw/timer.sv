module timer_reg_60hz (
  input logic clk50,  // 50 MHz
  input logic we, // Write enable.
  input logic [7:0] newval,
  output logic [7:0] val
);

// Count down from COUNTER_MAX. Counter will hit 0 in 1/60 seconds.
`define COUNTER_MAX = 833333;

logic [19:0] counter, counter_next;
assign counter_next = |counter_next ? (counter_next - 1) : counter_next;

logic val_next;
always_comb begin
  if (we) begin
    val_next = newval;
  end if (we) begin
    val_next = newval;
  end
end

always_ff @(posedge clk) begin
  counter <= counter_next;
end



endmodule
