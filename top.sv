module top
(
    input CLK,          // 25 MHz LCD clock
    input sclk,         // SPI clock from Pico
    input mosi,         // SPI data
    input cs_n,         // SPI chip select

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
// SPI Receiver - Receives command from Pico
// ============================================================
reg [7:0] received_value = 0;     // Stores the received number
reg [7:0] shift_reg = 0;
reg [2:0] bit_count = 0;
reg data_ready = 0;

always @(posedge sclk) begin
    if (cs_n) begin
        // Reset when not selected
        bit_count <= 0;
        shift_reg <= 0;
        data_ready <= 0;
    end else begin
        // Shift in SPI data (MSB first)
        shift_reg <= {shift_reg[6:0], mosi};
        bit_count <= bit_count + 1'b1;
        
        // After 8 bits, we have a full byte
        if (bit_count == 3'd7) begin
            received_value <= {shift_reg[6:0], mosi};
            data_ready <= 1'b1;
            bit_count <= 0;
        end else begin
            data_ready <= 0;
        end
    end
end

// ============================================================
// Command Interpretation
// 90 = vertical line
// anything else = horizontal line
// ============================================================
wire is_vertical = (received_value == 8'd90);
wire is_horizontal = (received_value != 8'd90);

// ============================================================
// Procedural Graphics - Draw line based on command
// ============================================================
// Vertical line: x = 234 to 245 (centered, 12 pixels wide)
wire vertical_line_active = is_vertical && 
                            (h_counter >= 234) && (h_counter <= 245);

// Horizontal line: y = 130 to 142 (centered, 12 pixels high)
wire horizontal_line_active = is_horizontal && 
                              (v_counter >= 130) && (v_counter <= 142);

// Line is active if either vertical or horizontal line is active
wire line_active = vertical_line_active || horizontal_line_active;

// Colors: Line = WHITE, Background = BLACK
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