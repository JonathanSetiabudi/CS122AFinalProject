    #include <stdio.h>
    #include <string.h>
    #include <math.h>
    #include "pico/stdlib.h"
    #include "pico/binary_info.h"
    #include "pico/time.h"
    #include "hardware/i2c.h"
    #include "hardware/adc.h"
    #include "hardware/spi.h"
    #include "hardware/timer.h"
    #include "hardware/gpio.h"
    #include "hardware/pwm.h"


    // I2C_SDA_PIN (pin 11) -> SDA on MPU6050 board
    // I2C_SCL_PIN (pin 12) -> SCL on MPU6050 board
    // 3.3v (pin 36) -> VCC on MPU6050 board
    // GND (pin 38)  -> GND on MPU6050 board
    // This code will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
    #define I2C_PORT i2c0
    #define I2C_SDA 8
    #define I2C_SCL 9
    #define POT_PIN 26
    #define SERVO_PIN 15

    #define SPI_CS_FPGA 17   
    #define SPI_CLK_PIN 18  
    #define SPI_MOSI_PIN 19  
    #define SPI_PORT spi0

    #define alpha 0.95

    // MPU6050 I2C address is 0x68.
    static int addr = 0x68;

    long map(long x, long in_min, long in_max, long out_min, long out_max) 
    {
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

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
            gyro[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);
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
        
    #if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
        #warning i2c/mpu6050_i2c example requires a board with I2C pins
        puts("Default I2C pins were not defined");
        return 0;
    #else
        printf("Hello, MPU6050! Reading raw data from registers...\n");

        adc_init();
        adc_gpio_init(POT_PIN);
        adc_select_input(0);

        i2c_init(I2C_PORT, 400*1000);
        
        gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
        gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
        gpio_pull_up(I2C_SDA);
        gpio_pull_up(I2C_SCL);
        bi_decl(bi_2pins_with_func(I2C_SDA, I2C_SCL, GPIO_FUNC_I2C));

        mpu6050_reset();

        int16_t acceleration[3], gyro[3], temp;
        float gyro_rate, accel_pitch;
        float angle_curr = 0.0;
        float angle_filtered = 0.0;
        float angle_desired = 0.0;
        float last_error = 0.0;
        float total_error = 0.0;
        float error = 0.0;
        float gainp = 1.0;
        float gaini = 0.05;
        float gaind = 0.2;
        float u = 0.0;
        uint potentiometer_max = 2000;
        uint potentiometer_min = 2000;
        uint16_t pot_value = 0;

        // ==========================================
        // POTENTIOMETER CALIBRATION ROUTINE
        // ==========================================
        for(int j = 0; j < 1000; j++){
            pot_value = adc_read(); // Discard first few readings to stabilize
            if (pot_value > potentiometer_max) potentiometer_max = pot_value;
            if (pot_value < potentiometer_min) potentiometer_min = pot_value;
            sleep_ms(3);
        }
        
        // ==========================================
        // GYRO CALIBRATION ROUTINE
        // ==========================================
        printf("Calibrating Gyro... Do not move the cannon!\n");
        int32_t gyro_x_sum = 0;
        
        for (int i = 0; i < 500; i++) {
            mpu6050_read_raw(acceleration, gyro, &temp);
            gyro_x_sum += gyro[0];
            sleep_ms(3);
        }
        
        float gyro_x_offset = (float)gyro_x_sum / 500.0;
        printf("Calibration complete. X-axis offset: %f\n", gyro_x_offset);
        sleep_ms(1000);
        
        // ==========================================
        // PWM SETUP FOR SERVO CONTROL
        // ==========================================
        gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
        uint slice_num = pwm_gpio_to_slice_num(SERVO_PIN);
        // Divide by 150 because default clock is 150 MHz and we want a 1MHz clock for our PWM to get the desired resolution for servo control. 
        // With a 1MHz clock and a wrap of 20000, we get a 20ms period which is standard for hobby servos.
        pwm_set_clkdiv(slice_num, 150.0f);
        pwm_set_wrap(slice_num, 20000);
        pwm_set_enabled(slice_num, true);
        
        // ==========================================
        // SPI SETUP FOR FPGA COMMUNICATION
        // ==========================================
        //250KHz SPI clock, Mode 0, MSB first
        spi_init(SPI_PORT, 250 * 1000);
        spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
        
        gpio_set_function(SPI_CLK_PIN, GPIO_FUNC_SPI);
        gpio_set_function(SPI_MOSI_PIN, GPIO_FUNC_SPI);
        
        gpio_init(SPI_CS_FPGA);
        gpio_set_dir(SPI_CS_FPGA, GPIO_OUT);
        gpio_put(SPI_CS_FPGA, 1);
        
        absolute_time_t next_loop_time = get_absolute_time();

        while (1) {
            next_loop_time = delayed_by_ms(next_loop_time, 20);  // 20ms loop time for ~50Hz update rate
            mpu6050_read_raw(acceleration, gyro, &temp);

            gyro_rate = -((float)(gyro[0] - gyro_x_offset) / 131.0);
            accel_pitch = -(atan2(acceleration[1], acceleration[2]) * (180.0 / M_PI));
            
            angle_curr = angle_filtered + (gyro_rate * 0.02); 
            angle_filtered = alpha * angle_curr + (1.0 - alpha) * accel_pitch;
            
            printf("Current filtered angle = %f\n", angle_filtered);
            
           
            pot_value = adc_read();
            // // Set angle desired to 0 degrees for PID tuning
            // angle_desired = 0.0;
            angle_desired = map(pot_value, potentiometer_min, potentiometer_max, 0, 90); // Map potentiometer reading to 0-90 degrees
            printf("Desired angle from potentiometer: %f\n", angle_desired);
            error = angle_desired - angle_filtered;

            if (error > -1.5 && error < 1.5) {
                error = 0.0; 
            }
            
            u = gainp * error + gaini * total_error + gaind * (error - last_error);

            total_error += error;
            // Safety Clamp: Don't let the integral memory explode past a useful limit
            if (total_error > 3000) total_error = 3000;
            if (total_error < -3000) total_error = -3000;

            last_error = error;
            
            float servo_target_angle = angle_desired + u; 

            // Constrain so we don't break the servo
            if (servo_target_angle < 0) servo_target_angle = 0;
            if (servo_target_angle > 180) servo_target_angle = 180;
            

            long pwm_level = map((long)servo_target_angle, 0, 180, 500, 2400);
            
            // Constrain PWM level
            if (pwm_level < 500) pwm_level = 500;
            if (pwm_level > 2400) pwm_level = 2400;
            
            printf("Servo target: %.1f°, PWM: %ld\n", servo_target_angle, pwm_level);
            
            // Command the servo
            pwm_set_gpio_level(SERVO_PIN, pwm_level);
            
            // Send SPI data to FPGA
            uint8_t spi_data[3];
            spi_data[0] = (uint8_t)(angle_filtered + 0.5);  // Angle (byte 1)
            spi_data[1] = (uint8_t)(angle_desired + 0.5);       // Target (byte 2)
            spi_data[2] = 0;                                    // Correction (byte 3, unused)
            gpio_put(SPI_CS_FPGA, 0);
            sleep_us(50);
            spi_write_blocking(SPI_PORT, spi_data, 3);
            sleep_us(50);
            gpio_put(SPI_CS_FPGA, 1);
            sleep_us(10);

            printf("Sent angle: %d, target: %d, correction: 0 to FPGA\n", spi_data[0], spi_data[1]);
            sleep_until(next_loop_time);
        }
    #endif
    }