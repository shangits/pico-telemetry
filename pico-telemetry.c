#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"


int main()
{
    stdio_init_all();
    sleep_ms(2000);
    cyw43_arch_init();
    
    // initialize adc sensor
    adc_init();
    adc_set_temp_sensor_enabled(true);

    // adc input 4 is RP2040's internal temp sensor
    adc_select_input(4);

    // tick counter for heartbeat
    int count = 0;

    // Blink loop
    while (true) {
        // hearbeat to confirm firmware works and USB port is functional
        printf("tick %d", count++);
        
        // convert adc sensor reading to volts then temp (deg C)
        float adc_volts = adc_read() * 3.3f / 4096;
        float temp = 27 - (adc_volts-0.706)/0.001721;
        printf(", temperature: %.3f\n", temp);

        
        // turn LED on
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(1000);

        // turn LED off
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(2000);
    }

}
