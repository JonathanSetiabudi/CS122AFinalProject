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
#define PACKED_SIZE   (TOTAL_PIXELS / 4)              // 32,640 bytes

// 2-bit color indices (only using 3 colors)
#define COLOR_BLACK  0
#define COLOR_RED    1
#define COLOR_YELLOW 2
#define COLOR_WHITE  3

// Packed framebuffer (4 pixels per byte!)
uint8_t framebuffer[PACKED_SIZE];

// Set a single pixel's 2-bit color
void set_pixel(int x, int y, int color) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    
    int pixel_num = y * SCREEN_WIDTH + x;
    int byte_index = pixel_num / 4;
    int bit_offset = (pixel_num % 4) * 2;
    
    framebuffer[byte_index] &= ~(0x03 << bit_offset);
    framebuffer[byte_index] |= (color & 0x03) << bit_offset;
}

// Fill entire screen with a color
void fill_screen(int color) {
    uint8_t packed_byte = 0;
    packed_byte |= (color & 0x03) << 0;
    packed_byte |= (color & 0x03) << 2;
    packed_byte |= (color & 0x03) << 4;
    packed_byte |= (color & 0x03) << 6;
    memset(framebuffer, packed_byte, PACKED_SIZE);
}

// Draw vertical line (full height)
void draw_vertical_line(int x_start, int width, int color) {
    for (int w = 0; w < width; w++) {
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            if (x_start + w < SCREEN_WIDTH) {
                set_pixel(x_start + w, y, color);
            }
        }
    }
}

// Draw horizontal line (full width)
void draw_horizontal_line(int y_start, int height, int color) {
    for (int h = 0; h < height; h++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            if (y_start + h < SCREEN_HEIGHT) {
                set_pixel(x, y_start + h, color);
            }
        }
    }
}

// Send packed framebuffer to FPGA over SPI
void spi_send_framebuffer(void) {
    printf("Sending %d bytes (packed 2-bit framebuffer)...\n", PACKED_SIZE);
    
    gpio_put(SPI_CS_FPGA, 0);
    sleep_us(2);
    
    spi_write_blocking(SPI_PORT, framebuffer, PACKED_SIZE);
    
    sleep_us(2);
    gpio_put(SPI_CS_FPGA, 1);
    
    printf("Done! %d pixels sent as %d bytes\n", TOTAL_PIXELS, PACKED_SIZE);
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

// Create vertical line test pattern
void create_vertical_line(void) {
    printf("Creating vertical line pattern...\n");
    
    // Fill with black
    fill_screen(COLOR_BLACK);
    
    // Draw vertical white line in center (12 pixels wide)
    int center_x = SCREEN_WIDTH / 2;  // 240
    int line_width = 12;
    int x_start = center_x - (line_width / 2);  // 234
    
    draw_vertical_line(x_start, line_width, COLOR_WHITE);
    
    // Add a red pixel at top center for debugging
    set_pixel(center_x, 10, COLOR_RED);
}

// Create horizontal line test pattern
void create_horizontal_line(void) {
    printf("Creating horizontal line pattern...\n");
    
    // Fill with black
    fill_screen(COLOR_BLACK);
    
    // Draw horizontal white line in center (12 pixels high)
    int center_y = SCREEN_HEIGHT / 2;  // 136
    int line_height = 12;
    int y_start = center_y - (line_height / 2);  // 130
    
    draw_horizontal_line(y_start, line_height, COLOR_WHITE);
    
    // Add a red pixel at left center for debugging
    set_pixel(10, center_y, COLOR_RED);
}

int main(void) {
    stdio_init_all();
    sleep_ms(1000);
    setup_spi_master();
    
    printf("\n=== 2-bit Packed Framebuffer Line Test ===\n");
    printf("Screen: %d x %d = %d pixels\n", SCREEN_WIDTH, SCREEN_HEIGHT, TOTAL_PIXELS);
    printf("Packed size: %d bytes (4 pixels per byte)\n", PACKED_SIZE);
    printf("Memory saved: %d bytes (%.1f%% reduction)\n", 
           TOTAL_PIXELS * 2 - PACKED_SIZE, 
           (1.0 - (float)PACKED_SIZE / (TOTAL_PIXELS * 2)) * 100);
    
    while (true) {
        // Send vertical line
        create_vertical_line();
        spi_send_framebuffer();
        printf("Vertical white line should appear!\n");
        sleep_ms(1000);
        
        // Send horizontal line
        create_horizontal_line();
        spi_send_framebuffer();
        printf("Horizontal white line should appear!\n");
        sleep_ms(1000);
    }
}