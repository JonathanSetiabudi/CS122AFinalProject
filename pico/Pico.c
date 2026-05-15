#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#define SPI_CS_FPGA 17
#define SPI_CLK_PIN 18
#define SPI_MOSI_PIN 19
#define SPI_PORT spi0

#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 272
#define TOTAL_PIXELS  (SCREEN_WIDTH * SCREEN_HEIGHT)  // 130,560 pixels

// RGB565 Colors
#define COLOR_BLACK  0x0000
#define COLOR_WHITE  0xFFFF
#define COLOR_RED    0xF800

// Send a single 16-bit pixel to FPGA
void spi_send_pixel(uint16_t pixel) {
    uint8_t bytes[2];
    bytes[0] = (pixel >> 8) & 0xFF;  // High byte first
    bytes[1] = pixel & 0xFF;         // Low byte second
    spi_write_blocking(SPI_PORT, bytes, 2);
}

// Send full framebuffer (generates vertical line on the fly)
void send_vertical_line_framebuffer(void) {
    int center_x = SCREEN_WIDTH / 2;   // 240
    int line_width = 12;
    int x_start = center_x - (line_width / 2);  // 234
    int x_end = x_start + line_width - 1;       // 245
    
    printf("Sending framebuffer with vertical line (x=%d to %d)...\n", x_start, x_end);
    
    gpio_put(SPI_CS_FPGA, 0);
    sleep_us(2);
    
    // Generate and send each pixel
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            uint16_t color = COLOR_BLACK;
            
            // Check if this pixel is in the vertical line
            if (x >= x_start && x <= x_end) {
                color = COLOR_WHITE;
            }
            
            // Add red debug pixel at top center
            if (x == center_x && y == 10) {
                color = COLOR_RED;
            }
            
            spi_send_pixel(color);
        }
    }
    
    sleep_us(2);
    gpio_put(SPI_CS_FPGA, 1);
    
    printf("Done! Sent %d pixels\n", TOTAL_PIXELS);
}

// Alternative: Send pre-built framebuffer (uses more RAM)
void send_prebuilt_framebuffer(void) {
    // This creates the full framebuffer in Pico RAM (261KB)
    // Use only if you have enough memory!
    
    uint16_t framebuffer[TOTAL_PIXELS];
    
    printf("Building framebuffer in RAM...\n");
    
    // Fill with black
    for (int i = 0; i < TOTAL_PIXELS; i++) {
        framebuffer[i] = COLOR_BLACK;
    }
    
    // Draw vertical white line
    int center_x = 240;
    int line_width = 12;
    int x_start = center_x - (line_width / 2);
    int x_end = x_start + line_width - 1;
    
    for (int x = x_start; x <= x_end; x++) {
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            framebuffer[y * SCREEN_WIDTH + x] = COLOR_WHITE;
        }
    }
    
    // Add red debug pixel
    framebuffer[10 * SCREEN_WIDTH + center_x] = COLOR_RED;
    
    printf("Sending %d bytes...\n", TOTAL_PIXELS * 2);
    
    gpio_put(SPI_CS_FPGA, 0);
    sleep_us(2);
    
    for (int i = 0; i < TOTAL_PIXELS; i++) {
        uint8_t bytes[2];
        bytes[0] = (framebuffer[i] >> 8) & 0xFF;
        bytes[1] = framebuffer[i] & 0xFF;
        spi_write_blocking(SPI_PORT, bytes, 2);
    }
    
    sleep_us(2);
    gpio_put(SPI_CS_FPGA, 1);
    
    printf("Done!\n");
}

// Setup SPI master
void setup_spi_master(void) {
    spi_init(SPI_PORT, 2500000);  // 2.5 MHz
    gpio_set_function(SPI_CLK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_MOSI_PIN, GPIO_FUNC_SPI);
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    
    gpio_init(SPI_CS_FPGA);
    gpio_set_dir(SPI_CS_FPGA, true);
    gpio_put(SPI_CS_FPGA, 1);
}

int main(void) {
    stdio_init_all();
    sleep_ms(1000);
    setup_spi_master();
    
    printf("\n=== Vertical Line Test with SDRAM Framebuffer ===\n");
    printf("Screen: %d x %d = %d pixels\n", SCREEN_WIDTH, SCREEN_HEIGHT, TOTAL_PIXELS);
    printf("SPI speed: 2.5 MHz\n");
    printf("Estimated transfer time: ~0.1 seconds\n");
    
    // Send the framebuffer (generates line on the fly - uses minimal RAM)
    send_vertical_line_framebuffer();
    
    printf("\nYou should see:\n");
    printf("  - Black background\n");
    printf("  - White vertical line (center, 12px wide, full height)\n");
    printf("  - Red pixel at top center of line\n");
    
    while (true) {
        tight_loop_contents();
    }
}