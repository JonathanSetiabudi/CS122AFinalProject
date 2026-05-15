module top (
    input  wire        CLK,        // Matches LPF
    input  wire        sclk,       // Matches LPF
    input  wire        mosi,       // Matches LPF
    input  wire        cs_n,       // Matches LPF
    
    output wire        LCD_CLK,    // Matches LPF
    output wire        LCD_DEN,    // Matches LPF
    output wire [4:0]  LCD_R,      // Matches LPF
    output wire [5:0]  LCD_G,      // Matches LPF
    output wire [4:0]  LCD_B,      // Matches LPF
    
    // SDRAM pins (must match LPF names)
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