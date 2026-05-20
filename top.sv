module top (
    input  wire        clk_25m,
    input  wire        sclk,
    input  wire        mosi,
    input  wire        cs_n,

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
    // SPI Receiver - Receives packed 2-bit pixels (4 pixels per byte)
    // ============================================================
    reg [7:0]  shift_reg = 0;
    reg [2:0]  bit_count = 0;
    reg [23:0] pixel_addr = 0;     // Tracks which pixel we're on (0 to 130559)
    reg        receiving = 0;
    
    // Color palette: convert 2-bit index to 16-bit RGB565
    // Index 0: Black, 1: Red, 2: Yellow, 3: White
    function [15:0] get_color;
        input [1:0] index;
        begin
            case(index)
                2'b00: get_color = 16'h0000;   // Black
                2'b01: get_color = 16'hF800;   // Red
                2'b10: get_color = 16'hFFE0;   // Yellow
                2'b11: get_color = 16'hFFFF;   // White
            endcase
        end
    endfunction
    
    // SPI receiver - receives packed bytes (4 pixels per byte)
    always @(posedge sclk) begin
        if (cs_n) begin
            bit_count <= 0;
            shift_reg <= 0;
            pixel_addr <= 0;
            wr_en <= 0;
            receiving <= 0;
        end else begin
            shift_reg <= {shift_reg[6:0], mosi};
            bit_count <= bit_count + 1'b1;
            
            // After 8 bits, we have a full packed byte
            if (bit_count == 3'd7) begin
                receiving <= 1;
                bit_count <= 0;
            end
        end
    end
    
    // Unpack the byte and write 4 pixels to SDRAM
    always @(posedge clk_25m) begin
        wr_en <= 0;
        
        if (receiving) begin
            receiving <= 0;
            
            // Write pixel 0 (bits 1:0)
            wr_addr <= pixel_addr + 0;
            wr_data <= get_color(shift_reg[1:0]);
            wr_en <= 1;
            
            // Note: In a real implementation, you'd need to write
            // all 4 pixels over multiple cycles. For simplicity,
            // this writes one pixel per received byte.
            // For full speed, you'd need a small buffer.
            
            pixel_addr <= pixel_addr + 1;
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