#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"


// I2C_SDA_PIN (pin 11) -> SDA on MPU6050 board
// I2C_SCL_PIN (pin 12) -> SCL on MPU6050 board
// 3.3v (pin 36) -> VCC on MPU6050 board
// GND (pin 38)  -> GND on MPU6050 board
// This code will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

// MPU6050 I2C address is 0x68.
static int addr = 0x68;

static void mpu6050_reset() {
    // Two byte reset. First byte register, second byte data
    // First we tell the device which register we want to write to (buf[0]) then we write the data (buf[1]) to that register. 
    // In this case, we want to write to the power management register (0x6B) and set the reset bit (0x80).
    uint8_t buf[] = {0x6B, 0x80};
    i2c_write_blocking(I2C_PORT, addr, buf, 2, false);
    sleep_ms(100); // Allow device to reset and stabilize

    // Clear sleep mode (0x6B register, 0x00 value)
    buf[1] = 0x00;  // Clear sleep mode by writing 0x00 to the 0x6B register
    i2c_write_blocking(I2C_PORT, addr, buf, 2, false); 
    sleep_ms(10); // Allow stabilization after waking up
}

static void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp) {
    // For this particular device, we send the device the register we want to read
    // first, then subsequently read from the device. The register is auto incrementing
    // so we don't need to keep sending the register we want, just the first.

    uint8_t buffer[6];

    // Start reading acceleration registers from register 0x3B for 6 bytes
    // Register 0x3B is the first of 6 registers that contain the acceleration data for x, y, and z axes (2 bytes each).
    uint8_t val = 0x3B;
    i2c_write_blocking(I2C_PORT, addr, &val, 1, true); // true to keep master control of bus
    i2c_read_blocking(I2C_PORT, addr, buffer, 6, false);

    // Buffer Format: [Accel_X_H, Accel_X_L, Accel_Y_H, Accel_Y_L, Accel_Z_H, Accel_Z_L]

    for (int i = 0; i < 3; i++) {
        // Combine the two bytes for each axis into a single 16-bit signed integer. The first byte is the high byte and the second byte is the low byte.
        accel[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);
    }

    // Now gyro data from reg 0x43 for 6 bytes
    // The register is auto incrementing on each read
    // Register 0x43 is the first of 6 registers that contain the gyroscope data for x, y, and z axes (2 bytes each).
    val = 0x43;
    i2c_write_blocking(I2C_PORT, addr, &val, 1, true);
    i2c_read_blocking(I2C_PORT, addr, buffer, 6, false);  // False - finished with bus

    for (int i = 0; i < 3; i++) {
        // Combine the two bytes for each axis into a single 16-bit signed integer. The first byte is the high byte and the second byte is the low byte.
        gyro[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);;
    }

    // Now temperature from reg 0x41 for 2 bytes
    // The register is auto incrementing on each read
    val = 0x41;
    i2c_write_blocking(I2C_PORT, addr, &val, 1, true);
    i2c_read_blocking(I2C_PORT, addr, buffer, 2, false);  // False - finished with bus

    *temp = buffer[0] << 8 | buffer[1];
}


int main()
{
    stdio_init_all();
    // Compile time check to ensure the board has I2C pins/hardware defined in the board file. If not, we can't run the code.
    // If any of these don't exist, the infinite while loopcode will not compile and will give a warning.
#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
    // This is the warning.
    #warning i2c/mpu6050_i2c example requires a board with I2C pins
    puts("Default I2C pins were not defined");
    return 0;
#else
    printf("Hello, MPU6050! Reading raw data from registers...\n");

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(I2C_SDA, I2C_SCL, GPIO_FUNC_I2C));
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c


    mpu6050_reset();

    int16_t acceleration[3], gyro[3], temp;

    while (1) {
        mpu6050_read_raw(acceleration, gyro, &temp);

        // These are the raw numbers from the chip, so will need tweaking to be really useful.
        // See the datasheet for more information
        printf("Acc. X = %d, Y = %d, Z = %d\n", acceleration[0], acceleration[1], acceleration[2]);
        printf("Gyro. X = %d, Y = %d, Z = %d\n", gyro[0], gyro[1], gyro[2]);
        // Temperature is simple so use the datasheet calculation to get deg C.
        // Note this is chip temperature.
        printf("Temp. = %f\n", (temp / 340.0) + 36.53);

        sleep_ms(100);
    }
#endif
}
