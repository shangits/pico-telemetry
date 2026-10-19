#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"



int main()
{
    stdio_init_all();
    sleep_ms(2000);
    cyw43_arch_init();

    // tick counter for heartbeat
    int count = 0;

    // Blink loop
    while (true) {
        // hearbeat to confirm firmware works and USB port is functional
        printf("tick %d\n", count++);
        
        // turn LED on
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(1000);

        // turn LED off
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(2000);
    }

}
