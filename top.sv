module top (
    // 25MHz clock
    input  wire        clk_25m,
    
    // SPI from Pico
    input  wire        sclk,
    input  wire        mosi,
    input  wire        cs_n,

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
    inout  wire [15:0] sdram_dq,

    // LCD Interface
    output wire        lcd_clk,
    output wire        lcd_hsync,
    output wire        lcd_vsync,
    output wire        lcd_de,
    output wire [4:0]  lcd_r,
    output wire [5:0]  lcd_g,
    output wire [4:0]  lcd_b,
    output wire [4:0]  dbg
);

    // ============================================================
    // Framebuffer Write Interface (Connects to Professor's Module)
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
    reg        receiving = 0;
    
    always @(posedge sclk) begin
        if (cs_n) begin
            // Reset when SPI not selected
            bit_count <= 0;
            shift_reg <= 0;
            pixel_addr <= 0;
            wr_en <= 0;
        end else begin
            // Shift in SPI data (MSB first)
            shift_reg <= {shift_reg[14:0], mosi};
            bit_count <= bit_count + 1'b1;
            
            // After 16 bits, we have a full pixel
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
    // Professor's LCD Framebuffer Module (Handles SDRAM + LCD)
    // ============================================================
    icesugar_pro_lcd_fb fb_inst (
        .clk_25m(clk_25m),
        
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

        .lcd_clk(lcd_clk),
        .lcd_hsync(lcd_hsync),
        .lcd_vsync(lcd_vsync),
        .lcd_de(lcd_de),
        .lcd_r(lcd_r),
        .lcd_g(lcd_g),
        .lcd_b(lcd_b),
        
        .wr_en(wr_en),
        .wr_addr(wr_addr),
        .wr_data(wr_data)
    );

    assign dbg = {lcd_hsync, lcd_vsync, lcd_de};

endmodule