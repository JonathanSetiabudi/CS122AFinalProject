#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#define SPI_CS_FPGA 17
#define SPI_CLK_PIN 18
#define SPI_MOSI_PIN 19
#define SPI_PORT spi0

// Commands
#define CMD_VERTICAL_LINE   0
#define CMD_HORIZONTAL_LINE 1

void send_command(uint8_t cmd) {
    gpio_put(SPI_CS_FPGA, 0);
    sleep_us(2);
    spi_write_blocking(SPI_PORT, &cmd, 1);
    sleep_us(2);
    gpio_put(SPI_CS_FPGA, 1);
}

void setup_spi_master(void) {
    spi_init(SPI_PORT, 2500000);
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
    
    printf("Starting line test - alternating every second...\n");
    
    while (true) {
        // Send vertical line command
        printf("Sending VERTICAL line command\n");
        send_command(CMD_VERTICAL_LINE);
        sleep_ms(1000);
        
        // Send horizontal line command
        printf("Sending HORIZONTAL line command\n");
        send_command(CMD_HORIZONTAL_LINE);
        sleep_ms(1000);
    }
}