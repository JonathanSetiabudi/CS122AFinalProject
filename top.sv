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
    // SPI Receiver - FINAL CORRECTED VERSION
    // ============================================================
    reg [15:0] shift_reg = 0;
    reg [3:0]  bit_count = 0;
    reg        fifo_wr_en_reg = 0;
    
    always @(posedge sclk) begin
        if (cs_n) begin
            bit_count <= 0;
            shift_reg <= 0;
            fifo_wr_en_reg <= 0;
        end else begin
            fifo_wr_en_reg <= 0;
            shift_reg <= {shift_reg[14:0], mosi};
            
            if (bit_count == 4'd15) begin
                fifo_wr_en_reg <= 1;
                bit_count <= 0;
            end else begin
                bit_count <= bit_count + 1'b1;
            end
        end
    end
    
    // ============================================================
    // Async FIFO (clock domain crossing)
    // ============================================================
    wire        fifo_wr_en;
    wire [15:0] fifo_din;
    wire        fifo_full;
    wire        fifo_rd_en;
    wire [15:0] fifo_dout;
    wire        fifo_empty;
    
    assign fifo_wr_en = fifo_wr_en_reg;
    assign fifo_din = {shift_reg[14:0], mosi};  // Complete 16-bit value
    
    async_fifo #(
        .DATA_WIDTH(16),
        .ADDR_WIDTH(9)
    ) spi_fifo (
        .wr_clk(sclk),
        .rd_clk(clk_25m),
        .rst(1'b0),
        .wr_en(fifo_wr_en && !fifo_full),
        .din(fifo_din),
        .rd_en(fifo_rd_en),
        .dout(fifo_dout),
        .empty(fifo_empty),
        .full(fifo_full),
        .almost_empty()
    );
    
    // ============================================================
    // FIFO Reader - writes to SDRAM (with registered data)
    // ============================================================
    reg [23:0] sdram_addr = 0;
    reg [1:0]  state = 0;
    reg        fifo_rd_en_reg = 0;
    reg [15:0] fifo_data_reg = 0;  // IMPORTANT: Register FIFO output
    
    assign fifo_rd_en = fifo_rd_en_reg;
    
    // Synchronize cs_n to clk_25m domain for frame reset
    reg cs_n_sync1 = 0;
    reg cs_n_sync2 = 0;
    reg cs_n_prev = 0;
    
    always @(posedge clk_25m) begin
        cs_n_sync1 <= cs_n;
        cs_n_sync2 <= cs_n_sync1;
        cs_n_prev <= cs_n_sync2;
        
        if (cs_n_sync2 && !cs_n_prev) begin
            sdram_addr <= 0;
        end
    end
    
    always @(posedge clk_25m) begin
        wr_en <= 0;
        fifo_rd_en_reg <= 0;
        
        case (state)
            0: begin
                if (!fifo_empty) begin
                    fifo_rd_en_reg <= 1;
                    state <= 1;
                end
            end
            
            1: begin
                // Register FIFO output to prevent it changing during write
                fifo_data_reg <= fifo_dout;
                state <= 2;
            end
            
            2: begin
                wr_addr <= sdram_addr;
                wr_data <= fifo_data_reg;  // Use registered value
                wr_en <= 1;
                sdram_addr <= sdram_addr + 1;
                state <= 0;
            end
        endcase
    end

    // ============================================================
    // Professor's SDRAM Framebuffer Module
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