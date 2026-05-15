#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#define SPI_CS_FPGA 17
#define SPI_CLK_PIN 18
#define SPI_MOSI_PIN 19
#define SPI_PORT spi0

// Send a single byte command to FPGA
void send_command(uint8_t cmd) {
    gpio_put(SPI_CS_FPGA, 0);
    sleep_us(2);
    spi_write_blocking(SPI_PORT, &cmd, 1);
    sleep_us(2);
    gpio_put(SPI_CS_FPGA, 1);
}

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
    
    printf("Starting line test...\n");
    printf("Sending 90 (vertical line)...\n");
    
    // Send 90 to draw VERTICAL line
    send_command(90);
    
    printf("Vertical line should appear!\n");
    printf("(Change code to send anything else for horizontal line)\n");
    
    // Optional: Cycle between vertical and horizontal every 3 seconds
    /*
    while (true) {
        printf("Vertical line (90)\n");
        send_command(90);
        sleep_ms(3000);
        
        printf("Horizontal line (0)\n");
        send_command(0);
        sleep_ms(3000);
    }
    */
    
    while (true) {
        tight_loop_contents();
    }
}