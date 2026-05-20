// #include <stdio.h>
// #include <string.h>
// #include <math.h>
// #include "pico/stdlib.h"
// #include "pico/binary_info.h"
// #include "hardware/i2c.h"



// // I2C_SDA_PIN (pin 11) -> SDA on MPU6050 board
// // I2C_SCL_PIN (pin 12) -> SCL on MPU6050 board
// // 3.3v (pin 36) -> VCC on MPU6050 board
// // GND (pin 38)  -> GND on MPU6050 board
// // This code will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// #define I2C_PORT i2c0
// #define I2C_SDA 8
// #define I2C_SCL 9

// #define alpha 0.98

// // MPU6050 I2C address is 0x68.
// static int addr = 0x68;

// static void mpu6050_reset() {
//     // Two byte reset. First byte register, second byte data
//     // First we tell the device which register we want to write to (buf[0]) then we write the data (buf[1]) to that register. 
//     // In this case, we want to write to the power management register (0x6B) and set the reset bit (0x80).
//     uint8_t buf[] = {0x6B, 0x80};
//     i2c_write_blocking(I2C_PORT, addr, buf, 2, false);
//     sleep_ms(100); // Allow device to reset and stabilize

//     // Clear sleep mode (0x6B register, 0x00 value)
//     buf[1] = 0x00;  // Clear sleep mode by writing 0x00 to the 0x6B register
//     i2c_write_blocking(I2C_PORT, addr, buf, 2, false); 
//     sleep_ms(10); // Allow stabilization after waking up
// }

// static void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp) {
//     // For this particular device, we send the device the register we want to read
//     // first, then subsequently read from the device. The register is auto incrementing
//     // so we don't need to keep sending the register we want, just the first.

//     uint8_t buffer[6];

//     // Start reading acceleration registers from register 0x3B for 6 bytes
//     // Register 0x3B is the first of 6 registers that contain the acceleration data for x, y, and z axes (2 bytes each).
//     uint8_t val = 0x3B;
//     i2c_write_blocking(I2C_PORT, addr, &val, 1, true); // true to keep master control of bus
//     i2c_read_blocking(I2C_PORT, addr, buffer, 6, false);

//     // Buffer Format: [Accel_X_H, Accel_X_L, Accel_Y_H, Accel_Y_L, Accel_Z_H, Accel_Z_L]

//     for (int i = 0; i < 3; i++) {
//         // Combine the two bytes for each axis into a single 16-bit signed integer. The first byte is the high byte and the second byte is the low byte.
//         accel[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);
//     }

//     // Now gyro data from reg 0x43 for 6 bytes
//     // The register is auto incrementing on each read
//     // Register 0x43 is the first of 6 registers that contain the gyroscope data for x, y, and z axes (2 bytes each).
//     val = 0x43;
//     i2c_write_blocking(I2C_PORT, addr, &val, 1, true);
//     i2c_read_blocking(I2C_PORT, addr, buffer, 6, false);  // False - finished with bus

//     for (int i = 0; i < 3; i++) {
//         // Combine the two bytes for each axis into a single 16-bit signed integer. The first byte is the high byte and the second byte is the low byte.
//         gyro[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);;
//     }

//     // Now temperature from reg 0x41 for 2 bytes
//     // The register is auto incrementing on each read
//     val = 0x41;
//     i2c_write_blocking(I2C_PORT, addr, &val, 1, true);
//     i2c_read_blocking(I2C_PORT, addr, buffer, 2, false);  // False - finished with bus

//     *temp = buffer[0] << 8 | buffer[1];
// }


// int main()
// {
//     stdio_init_all();
//     // Compile time check to ensure the board has I2C pins/hardware defined in the board file. If not, we can't run the code.
//     // If any of these don't exist, the infinite while loopcode will not compile and will give a warning.
// #if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
//     // This is the warning.
//     #warning i2c/mpu6050_i2c example requires a board with I2C pins
//     puts("Default I2C pins were not defined");
//     return 0;
// #else
//     printf("Hello, MPU6050! Reading raw data from registers...\n");

//     // I2C Initialisation. Using it at 400Khz.
//     i2c_init(I2C_PORT, 400*1000);
    
//     gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
//     gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
//     gpio_pull_up(I2C_SDA);
//     gpio_pull_up(I2C_SCL);
//     // Make the I2C pins available to picotool
//     bi_decl(bi_2pins_with_func(I2C_SDA, I2C_SCL, GPIO_FUNC_I2C));
//     // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c


//     mpu6050_reset();

//     int16_t acceleration[3], gyro[3], temp;
//     float gyro_rate, accel_pitch;
//     float angle_curr = 0.0;
//     float angle_filtered = 0.0;
//     while (1) {
//         mpu6050_read_raw(acceleration, gyro, &temp);

//         // These are the raw numbers from the chip, so will need tweaking to be really useful.
//         // See the datasheet for more information
//         // printf("Acc. X = %d, Y = %d, Z = %d\n", acceleration[0], acceleration[1], acceleration[2]);
//         // printf("Gyro. X = %d, Y = %d, Z = %d\n", gyro[0], gyro[1], gyro[2]);
//         // // Temperature is simple so use the datasheet calculation to get deg C.
//         // // Note this is chip temperature.
//         // printf("Temp. = %f\n", (temp / 340.0) + 36.53);

//         // Convert raw gyroscope data to degrees per second
//         gyro_rate = (float)gyro[0] / 131.0;
//         accel_pitch = atan2(-acceleration[0],sqrt((acceleration[1]*acceleration[1] + acceleration[2]*acceleration[2]))) * (180.0/M_PI);
//         // dt is 100ms or 0.1 seconds
//         angle_curr = angle_filtered + (gyro_rate * 0.1);
//         angle_filtered = alpha * angle_curr + (1-alpha) * accel_pitch;
//         printf("Current unfiltered angle = %f\n", angle_curr);
//         printf("Current filtered angle = %f\n", angle_filtered);
//         sleep_ms(100);
//     }
// #endif
// }
#include <stdio.h>
#include <math.h>
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
    int dx = fabs(x1 - x0);
    int dy = -fabs(y1 - y0);
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