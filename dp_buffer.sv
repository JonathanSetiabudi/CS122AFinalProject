module dp_buffer #(
    parameter DEPTH = 32640,   // 480*272/4 = 32640 bytes
    parameter WIDTH = 8,       // 8 bits per address (4 pixels packed)
    parameter ADDR_W = 15      // 2^15 = 32768 > 32640
) (
    // Port A: SPI writes (sclk domain)
    input  logic clk_a,
    input  logic we_a,
    input  logic [ADDR_W-1:0] addr_a,
    input  logic [WIDTH-1:0]  data_a,
    
    // Port B: LCD reads (CLK domain)
    input  logic clk_b,
    input  logic [ADDR_W-1:0] addr_b,
    output logic [WIDTH-1:0]  data_b
);
    logic [WIDTH-1:0] mem [0:DEPTH-1];
    
    // Write port (sclk domain)
    always_ff @(posedge clk_a) begin
        if (we_a) begin
            mem[addr_a] <= data_a;
        end
    end
    
    // Read port (CLK domain)
    always_ff @(posedge clk_b) begin
        data_b <= mem[addr_b];
    end

endmodule