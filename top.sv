module top (
    // iCESugar-Pro 25MHz onboard clock (Pin P6)
    input  wire        clk_25m,
    
    // SPI from Pico
    input  wire        sclk,
    input  wire        mosi,
    input  wire        cs_n,

    // IS42S16160B SDRAM Interface
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

    // RGB LCD Interface (480x272)
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
    // Framebuffer Write Interface
    // ============================================================
    reg        wr_en = 0;
    reg [23:0] wr_addr = 0;
    reg [15:0] wr_data = 0;

    // ============================================================
    // SPI Receiver - Receives command from Pico
    // ============================================================
    reg [7:0] received_command = 0;
    reg [7:0] shift_reg = 0;
    reg [2:0] bit_count = 0;
    reg command_ready = 0;
    reg [23:0] framebuffer_addr = 0;
    
    // Colors
    localparam COLOR_BLACK = 16'h0000;
    localparam COLOR_WHITE = 16'hFFFF;
    
    // Drawing state
    reg [1:0] line_type = 0;  // 0 = vertical, 1 = horizontal
    reg [31:0] timer = 0;
    reg draw_complete = 0;
    
    // SPI receiver
    always @(posedge sclk) begin
        if (cs_n) begin
            bit_count <= 0;
            shift_reg <= 0;
            command_ready <= 0;
        end else begin
            shift_reg <= {shift_reg[6:0], mosi};
            bit_count <= bit_count + 1'b1;
            
            if (bit_count == 4'd7) begin
                received_command <= {shift_reg[6:0], mosi};
                command_ready <= 1'b1;
                bit_count <= 0;
            end else begin
                command_ready <= 0;
            end
        end
    end
    
    // Process command and draw to framebuffer
    always @(posedge clk_25m) begin
        // Timer to alternate lines every second (25MHz = 25,000,000 cycles)
        timer <= timer + 1;
        
        if (timer >= 25000000) begin
            timer <= 0;
            draw_complete <= 0;
            
            // Switch line type
            if (line_type == 0) begin
                line_type <= 1;  // Switch to horizontal
            end else begin
                line_type <= 0;  // Switch to vertical
            end
        end
        
        // Draw when not complete
        if (!draw_complete) begin
            wr_en <= 1;
            
            // Fill entire framebuffer with black first
            if (framebuffer_addr < 130560) begin
                wr_addr <= framebuffer_addr;
                wr_data <= COLOR_BLACK;
                framebuffer_addr <= framebuffer_addr + 1;
            end
            // Then draw the line
            else if (line_type == 0 && framebuffer_addr >= 130560 && framebuffer_addr < 130560 + 480*272) begin
                // Draw VERTICAL line (x = 234 to 245, all y)
                // We need to write pixels at x=234-245 for all y
                // Address = y * 480 + x
                // This is simplified - we'll loop through y and x
                // For now, just draw a few lines to test
                if (framebuffer_addr < 130560 + 480) begin
                    // Draw one horizontal row at a time
                    wr_addr <= framebuffer_addr;
                    wr_data <= COLOR_WHITE;
                    framebuffer_addr <= framebuffer_addr + 1;
                end else begin
                    draw_complete <= 1;
                    framebuffer_addr <= 0;
                end
            end
            else if (line_type == 1 && framebuffer_addr >= 130560 && framebuffer_addr < 130560 + 480*272) begin
                // Draw HORIZONTAL line (y = 130 to 142, all x)
                if (framebuffer_addr < 130560 + 480) begin
                    wr_addr <= framebuffer_addr;
                    wr_data <= COLOR_WHITE;
                    framebuffer_addr <= framebuffer_addr + 1;
                end else begin
                    draw_complete <= 1;
                    framebuffer_addr <= 0;
                end
            end
            else begin
                wr_en <= 0;
            end
        end else begin
            wr_en <= 0;
        end
    end

    // Professor's SDRAM Framebuffer Module
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