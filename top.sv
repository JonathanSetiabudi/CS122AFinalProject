module top (
    input  wire        CLK,        // 25MHz clock
    input  wire        sclk,       // SPI clock from Pico
    input  wire        mosi,       // SPI data
    input  wire        cs_n,       // SPI chip select

    // LCD Outputs
    output wire        LCD_CLK,
    output wire        LCD_DEN,
    output wire [4:0]  LCD_R,
    output wire [5:0]  LCD_G,
    output wire [4:0]  LCD_B,

    // SDRAM Interface
    output wire        sdram_clk,
    output wire        sdram_cke,
    output wire        sdram_cs_n,
    output wire        sdram_ras_n,
    output wire        sdram_cas_n,
    output wire        sdram_we_n,
    output wire [1:0]  sdram_ba,
    output wire [12:0] sdram_a,
    output wire [1:0]  sdram_dqm,
    inout  wire [15:0] sdram_dq
);

    reg        wr_en = 0;
    reg [23:0] wr_addr = 0;
    reg [15:0] wr_data = 0;
    reg [15:0] shift_reg = 0;
    reg [3:0]  bit_count = 0;
    reg [23:0] pixel_addr = 0;
    
    always @(posedge sclk) begin
        if (cs_n) begin
            bit_count <= 0;
            shift_reg <= 0;
            pixel_addr <= 0;
            wr_en <= 0;
        end else begin
            shift_reg <= {shift_reg[14:0], mosi};
            bit_count <= bit_count + 1'b1;
            
            if (bit_count == 4'd15) begin
                wr_en <= 1;
                wr_addr <= pixel_addr;
                wr_data <= {shift_reg[14:0], mosi};
                pixel_addr <= pixel_addr + 1;
                bit_count <= 0;
            end else begin
                wr_en <= 0;
            end
        end
    end

    wire        lcd_clk_int;
    wire        lcd_de_int;
    wire [4:0]  lcd_r_int;
    wire [5:0]  lcd_g_int;
    wire [4:0]  lcd_b_int;
    
    icesugar_pro_lcd_fb fb_inst (
        .clk_25m(CLK),
        
        .sdram_clk(sdram_clk),
        .sdram_cke(sdram_cke),
        .sdram_cs_n(sdram_cs_n),
        .sdram_ras_n(sdram_ras_n),
        .sdram_cas_n(sdram_cas_n),
        .sdram_we_n(sdram_we_n),
        .sdram_ba(sdram_ba),
        .sdram_a(sdram_a),
        .sdram_dqm(sdram_dqm),
        .sdram_dq(sdram_dq),

        .lcd_clk(lcd_clk_int),
        .lcd_hsync(),
        .lcd_vsync(),
        .lcd_de(lcd_de_int),
        .lcd_r(lcd_r_int),
        .lcd_g(lcd_g_int),
        .lcd_b(lcd_b_int),
        
        .wr_en(wr_en),
        .wr_addr(wr_addr),
        .wr_data(wr_data)
    );
    
    // Connect to your LCD pins
    assign LCD_CLK = lcd_clk_int;
    assign LCD_DEN = lcd_de_int;
    assign LCD_R = lcd_r_int;
    assign LCD_G = lcd_g_int;
    assign LCD_B = lcd_b_int;

endmodule