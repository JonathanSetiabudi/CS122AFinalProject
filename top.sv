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
    // SPI Receiver - 16-bit RGB565 pixels
    // ============================================================
    reg [15:0] shift_reg = 0;
    reg [3:0]  bit_count = 0;
    reg [23:0] pixel_addr = 0;
    reg        write_pending = 0;
    reg [15:0] pending_data = 0;
    reg [23:0] pending_addr = 0;
    
    // SPI receiver on sclk domain
    always @(posedge sclk) begin
        if (cs_n) begin
            bit_count <= 0;
            shift_reg <= 0;
            pixel_addr <= 0;
            write_pending <= 0;
        end else begin
            shift_reg <= {shift_reg[14:0], mosi};
            bit_count <= bit_count + 1'b1;
            
            if (bit_count == 4'd15) begin
                // Full 16-bit pixel received
                pending_data <= {shift_reg[14:0], mosi};
                pending_addr <= pixel_addr;
                write_pending <= 1;
                pixel_addr <= pixel_addr + 1;
                bit_count <= 0;
            end
        end
    end
    
    // Cross to clk_25m domain with simple synchronizer
    reg        write_pending_sync = 0;
    reg [15:0] pending_data_sync = 0;
    reg [23:0] pending_addr_sync = 0;
    
    always @(posedge clk_25m) begin
        write_pending_sync <= write_pending;
        if (write_pending_sync) begin
            pending_data_sync <= pending_data;
            pending_addr_sync <= pending_addr;
        end
    end
    
    // Write to SDRAM on clk_25m domain
    reg write_done = 0;
    
    always @(posedge clk_25m) begin
        wr_en <= 0;
        
        if (write_pending_sync && !write_done) begin
            wr_addr <= pending_addr_sync;
            wr_data <= pending_data_sync;
            wr_en <= 1;
            write_done <= 1;
        end else begin
            write_done <= 0;
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