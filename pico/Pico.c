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
#define PACKED_SIZE   ((SCREEN_WIDTH * SCREEN_HEIGHT) / 4)  // 32640 bytes

// Color indices (2 bits)
#define COLOR_BLACK  0
#define COLOR_RED    1
#define COLOR_YELLOW 2
#define COLOR_WHITE  3

// Framebuffer (packed: 4 pixels per byte)
uint8_t framebuffer[PACKED_SIZE];

// Set a single pixel's color (x: 0-479, y: 0-271)
void set_pixel(int x, int y, int color) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    
    int pixel_index = y * SCREEN_WIDTH + x;
    int byte_index = pixel_index / 4;
    int bit_offset = (pixel_index % 4) * 2;
    
    framebuffer[byte_index] &= ~(0x03 << bit_offset);
    framebuffer[byte_index] |= (color & 0x03) << bit_offset;
}

// Fill entire screen with a single color
void fill_screen(int color) {
    uint8_t packed_color = 0;
    packed_color |= (color & 0x03) << 0;
    packed_color |= (color & 0x03) << 2;
    packed_color |= (color & 0x03) << 4;
    packed_color |= (color & 0x03) << 6;
    memset(framebuffer, packed_color, PACKED_SIZE);
}

// Draw vertical line (x constant, y from start to end)
void draw_vertical_line(int x, int y_start, int y_end, int color) {
    for (int y = y_start; y <= y_end; y++) {
        set_pixel(x, y, color);
    }
}

// Draw horizontal line (y constant, x from start to end)
void draw_horizontal_line(int x_start, int x_end, int y, int color) {
    for (int x = x_start; x <= x_end; x++) {
        set_pixel(x, y, color);
    }
}

// Send framebuffer to FPGA over SPI
void spi_send_framebuffer(void) {
    gpio_put(SPI_CS_FPGA, 0);
    sleep_us(2);
    
    spi_write_blocking(SPI_PORT, framebuffer, PACKED_SIZE);
    
    sleep_us(2);
    gpio_put(SPI_CS_FPGA, 1);
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

// Create vertical white line test pattern
void create_vertical_line_test(void) {
    // Start with all black
    fill_screen(COLOR_BLACK);
    
    // Draw vertical white line in the center (12 pixels wide)
    int center_x = SCREEN_WIDTH / 2;    // 240
    int line_width = 12;
    int x_start = center_x - (line_width / 2);   // 234
    int x_end = center_x + (line_width / 2) - 1; // 245
    
    for (int x = x_start; x <= x_end; x++) {
        draw_vertical_line(x, 0, SCREEN_HEIGHT - 1, COLOR_WHITE);
    }
    
    // Draw a red horizontal line at the top for debugging
    draw_horizontal_line(0, SCREEN_WIDTH - 1, 10, COLOR_RED);
    
    // Draw a yellow dot at the exact center
    set_pixel(center_x, SCREEN_HEIGHT / 2, COLOR_YELLOW);
}

// Print framebuffer info (debugging)
void print_framebuffer_info(void) {
    printf("Framebuffer size: %d bytes\n", PACKED_SIZE);
    printf("Screen: %d x %d = %d pixels\n", SCREEN_WIDTH, SCREEN_HEIGHT, 
           SCREEN_WIDTH * SCREEN_HEIGHT);
    printf("Bits per pixel: 2\n");
    printf("Colors: Black(0), Red(1), Yellow(2), White(3)\n");
}

int main(void) {
    stdio_init_all();
    sleep_ms(1000);
    setup_spi_master();
    
    printf("\n=== Vertical Line Test ===\n");
    print_framebuffer_info();
    
    // Create test pattern
    printf("Creating test pattern...\n");
    create_vertical_line_test();
    
    // Send to FPGA
    printf("Sending %d bytes to FPGA...\n", PACKED_SIZE);
    spi_send_framebuffer();
    
    printf("Done! You should see:\n");
    printf("  - Black background\n");
    printf("  - 12-pixel wide vertical WHITE line in center\n");
    printf("  - RED horizontal line at y=10\n");
    printf("  - YELLOW dot at exact center\n");
    
    while (true) {
        tight_loop_contents();
    }
    
    return 0;
}