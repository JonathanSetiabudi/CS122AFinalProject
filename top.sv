module top
(
    input CLK,          // 25 MHz LCD clock

    output LCD_CLK,
    output LCD_DEN,
    output [4:0] LCD_B,
    output [5:0] LCD_G,
    output [4:0] LCD_R
);

// ============================================================
// LCD Timing (480x272 with blanking)
// ============================================================
reg [9:0] h_counter = 0;
reg [8:0] v_counter = 0;

wire h_active = (h_counter < 480);
wire v_active = (v_counter < 272);
wire active = h_active & v_active;

assign LCD_CLK = CLK;
assign LCD_DEN = active;

// ============================================================
// Procedural Graphics - Vertical White Line
// ============================================================
// Line parameters:
//   Center at x = 240
//   Width = 12 pixels (x = 234 to 245)
//   Full height (y = 0 to 271)

wire line_active = (h_counter >= 234) && (h_counter <= 245);

// Background is BLACK, line is WHITE
wire [4:0] red   = line_active ? 5'b11111 : 5'b00000;
wire [5:0] green = line_active ? 6'b111111 : 6'b000000;
wire [4:0] blue  = line_active ? 5'b11111 : 5'b00000;

// Output to LCD (only during active region)
assign LCD_R = active ? red   : 5'b00000;
assign LCD_G = active ? green : 6'b000000;
assign LCD_B = active ? blue  : 5'b00000;

// ============================================================
// LCD Scanline Counter
// ============================================================
always @(posedge CLK) begin
    if (h_counter == 524) begin
        h_counter <= 0;
        if (v_counter == 284)
            v_counter <= 0;
        else
            v_counter <= v_counter + 1'b1;
    end else begin
        h_counter <= h_counter + 1'b1;
    end
end

endmodule