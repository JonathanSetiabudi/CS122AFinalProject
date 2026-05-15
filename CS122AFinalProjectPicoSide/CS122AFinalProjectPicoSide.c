// #include <stdio.h>
// #include <string.h>
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

//     while (1) {
//         mpu6050_read_raw(acceleration, gyro, &temp);

//         // These are the raw numbers from the chip, so will need tweaking to be really useful.
//         // See the datasheet for more information
//         printf("Acc. X = %d, Y = %d, Z = %d\n", acceleration[0], acceleration[1], acceleration[2]);
//         printf("Gyro. X = %d, Y = %d, Z = %d\n", gyro[0], gyro[1], gyro[2]);
//         // Temperature is simple so use the datasheet calculation to get deg C.
//         // Note this is chip temperature.
//         printf("Temp. = %f\n", (temp / 340.0) + 36.53);

//         sleep_ms(100);
//     }
// #endif
// }
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