module mymodule(input logic clk,
  input logic do_count,
  output logic [1:0] counter);


initial begin
  counter = 2'b0;
end

always @(posedge clk) begin
  if (do_count) begin
    counter <= counter + 2'b1;
  end
end

endmodule
