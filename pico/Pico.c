#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#define SPI_CS_FPGA 8   // GPIO 8 (Pin 11)
#define SPI_CLK_PIN 6   // GPIO 6 (Pin 9)
#define SPI_MOSI_PIN 7  // GPIO 7 (Pin 10)
#define SPI_PORT spi0

#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 272
#define TOTAL_PIXELS  (SCREEN_WIDTH * SCREEN_HEIGHT)  // 130,560 pixels

// RGB565 Colors
#define COLOR_BLACK  0x0000
#define COLOR_WHITE  0xFFFF
#define COLOR_RED    0xF800
#define COLOR_GREEN  0x07E0
#define COLOR_BLUE   0x001F
#define COLOR_YELLOW 0xFFE0

// Framebuffer (130,560 pixels × 2 bytes = 261,120 bytes)
static uint16_t framebuffer[TOTAL_PIXELS];

// Set a pixel in the framebuffer
void set_pixel(int x, int y, uint16_t color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        framebuffer[y * SCREEN_WIDTH + x] = color;
    }
}

// Fill entire framebuffer with a color
void fill_screen(uint16_t color) {
    for (int i = 0; i < TOTAL_PIXELS; i++) {
        framebuffer[i] = color;
    }
}

// Draw vertical line (full height)
void draw_vertical_line(int x_start, int width, uint16_t color) {
    for (int w = 0; w < width; w++) {
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            if (x_start + w < SCREEN_WIDTH) {
                set_pixel(x_start + w, y, color);
            }
        }
    }
}

// Draw horizontal line (full width)
void draw_horizontal_line(int y_start, int height, uint16_t color) {
    for (int h = 0; h < height; h++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            if (y_start + h < SCREEN_HEIGHT) {
                set_pixel(x, y_start + h, color);
            }
        }
    }
}

// Draw a circle (for compass gauge)
void draw_circle(int cx, int cy, int radius, uint16_t color) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) {
                set_pixel(cx + x, cy + y, color);
            }
        }
    }
}

// Draw a line using Bresenham's algorithm
void draw_line(int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;
    
    while (1) {
        set_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// Send framebuffer to FPGA over SPI
void spi_send_framebuffer(void) {
    gpio_put(SPI_CS_FPGA, 0);
    sleep_us(2);
    
    // Send each pixel as 16-bit (MSB first)
    for (int i = 0; i < TOTAL_PIXELS; i++) {
        uint8_t bytes[2];
        bytes[0] = (framebuffer[i] >> 8) & 0xFF;
        bytes[1] = framebuffer[i] & 0xFF;
        spi_write_blocking(SPI_PORT, bytes, 2);
    }
    
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

// Create vertical line test pattern
void create_vertical_line(void) {
    fill_screen(COLOR_BLACK);
    int center_x = SCREEN_WIDTH / 2;  // 240
    int line_width = 12;
    int x_start = center_x - (line_width / 2);  // 234
    draw_vertical_line(x_start, line_width, COLOR_WHITE);
    set_pixel(center_x, 10, COLOR_RED);  // Debug pixel
}

// Create horizontal line test pattern
void create_horizontal_line(void) {
    fill_screen(COLOR_BLACK);
    int center_y = SCREEN_HEIGHT / 2;  // 136
    int line_height = 12;
    int y_start = center_y - (line_height / 2);  // 130
    draw_horizontal_line(y_start, line_height, COLOR_WHITE);
    set_pixel(10, center_y, COLOR_RED);  // Debug pixel
}

// Create compass gauge for accelerometer
void create_compass_gauge(int angle_deg) {
    fill_screen(COLOR_BLACK);
    
    int cx = SCREEN_WIDTH / 2;   // 240
    int cy = SCREEN_HEIGHT / 2;  // 136
    int radius = 100;
    
    // Draw compass circle
    draw_circle(cx, cy, radius, COLOR_WHITE);
    
    // Draw tick marks (every 30 degrees)
    for (int deg = -90; deg <= 90; deg += 30) {
        float rad = deg * 3.14159f / 180.0f;
        int x1 = cx + (int)((radius - 10) * sin(rad));
        int y1 = cy - (int)((radius - 10) * cos(rad));
        int x2 = cx + (int)(radius * sin(rad));
        int y2 = cy - (int)(radius * cos(rad));
        draw_line(x1, y1, x2, y2, COLOR_WHITE);
    }
    
    // Draw needle at specified angle
    float rad = angle_deg * 3.14159f / 180.0f;
    int needle_x = cx + (int)((radius - 20) * sin(rad));
    int needle_y = cy - (int)((radius - 20) * cos(rad));
    draw_line(cx, cy, needle_x, needle_y, COLOR_RED);
    
    // Draw center dot
    draw_circle(cx, cy, 5, COLOR_WHITE);
}

// Simulate accelerometer (replace with actual I2C reading)
int read_accelerometer_angle(void) {
    static int angle = 0;
    static int direction = 1;
    
    angle += direction;
    if (angle >= 90 || angle <= -90) {
        direction = -direction;
    }
    return angle;
}

int main(void) {
    stdio_init_all();
    sleep_ms(1000);
    setup_spi_master();
    
    printf("\n=== FPGA Framebuffer Test ===\n");
    printf("Screen: %d x %d = %d pixels\n", SCREEN_WIDTH, SCREEN_HEIGHT, TOTAL_PIXELS);
    printf("Framebuffer size: %d bytes\n", TOTAL_PIXELS * 2);
    
    int mode = 0;  // 0=vertical, 1=horizontal, 2=gauge
    int angle = 0;
    
    while (1) {
        switch (mode) {
            case 0:
                printf("Vertical line test\n");
                create_vertical_line();
                spi_send_framebuffer();
                sleep_ms(2000);
                mode = 1;
                break;
                
            case 1:
                printf("Horizontal line test\n");
                create_horizontal_line();
                spi_send_framebuffer();
                sleep_ms(2000);
                mode = 2;
                break;
                
            case 2:
                // Read actual accelerometer here
                angle = read_accelerometer_angle();
                printf("Compass gauge: angle = %d°\n", angle);
                create_compass_gauge(angle);
                spi_send_framebuffer();
                sleep_ms(100);
                // Stay in gauge mode, just update angle
                break;
        }
    }
}