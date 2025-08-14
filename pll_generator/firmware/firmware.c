#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <math.h>

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19



int main()
{
    stdio_init_all();

    // SPI initialisation. This example will use SPI at 1MHz.
    spi_init(SPI_PORT, 1000*1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS,   GPIO_FUNC_SIO);

    adf_init(SPI_PORT);

    while (true) {
        char c = getchar_timeout_us(10000);
        if (c == PICO_ERROR_TIMEOUT) {
            continue;
        }
        printf("Received character: %c\n", c);
        if (c == 'f') {
            // example: set 100 MHz
            adf_set_frequency(100e6);
            sleep_ms(50);
    
            printf("Locked? %s\n", adf_is_locked() ? "YES" : "NO");
            continue;
        }
        if (c == 's') {
            for (double f = 50e6; f <= 500e6; f += 50e6) {
                adf_set_frequency(f);
                sleep_ms(200);
                printf("Set %.0f kHz - Locked: %d\n", f/1e3, adf_is_locked());
            }
        }
        
        sleep_ms(500);
    }
}
