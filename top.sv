module top (
    // Your existing clock name
    input  wire        CLK,        // Was clk_25m
    
    // SPI from Pico (matches your LPF)
    input  wire        sclk,
    input  wire        mosi,
    input  wire        cs_n,

    // SDRAM Interface (keep same)
    output wire        sdram_clk,
    output wire        sdram_cke,
    output wire        sdram_cs_n,
    output wire        sdram_ras_n,
    output wire        sdram_cas_n,
    output wire        sdram_we_n,
    output wire [1:0]  sdram_ba,
    output wire [12:0] sdram_a,
    output wire [1:0]  sdram_dqm,
    inout  wire [15:0] sdram_dq,

    // LCD Interface (match YOUR existing pin names)
    output wire        LCD_CLK,    // Was lcd_clk
    output wire        LCD_DEN,    // Was lcd_de
    output wire [4:0]  LCD_R,      // Was lcd_r
    output wire [5:0]  LCD_G,      // Was lcd_g
    output wire [4:0]  LCD_B       // Was lcd_b
);

    // ============================================================
    // Framebuffer Write Interface
    // ============================================================
    reg        wr_en;
    reg [23:0] wr_addr;
    reg [15:0] wr_data;

    // ============================================================
    // SPI Receiver - Receives 16-bit RGB565 pixels from Pico
    // ============================================================
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

    // ============================================================
    // Professor's SDRAM Framebuffer Module
    // Need to adapt signal names to match
    // ============================================================
    
    // Generate internal signals for professor's module
    wire        lcd_clk_int;
    wire        lcd_de_int;
    wire [4:0]  lcd_r_int;
    wire [5:0]  lcd_g_int;
    wire [4:0]  lcd_b_int;
    
    icesugar_pro_lcd_fb fb_inst (
        .clk_25m(CLK),              // Map CLK to clk_25m
        
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
        .lcd_hsync(),               // Not used in your LCD
        .lcd_vsync(),               // Not used in your LCD
        .lcd_de(lcd_de_int),
        .lcd_r(lcd_r_int),
        .lcd_g(lcd_g_int),
        .lcd_b(lcd_b_int),
        
        .wr_en(wr_en),
        .wr_addr(wr_addr),
        .wr_data(wr_data)
    );
    
    // Connect professor's outputs to your pin names
    assign LCD_CLK = lcd_clk_int;
    assign LCD_DEN = lcd_de_int;
    assign LCD_R = lcd_r_int;
    assign LCD_G = lcd_g_int;
    assign LCD_B = lcd_b_int;

endmodule