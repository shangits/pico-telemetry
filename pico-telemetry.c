#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
#define WINDOW_SIZE 16

// initializing variables for ring buffer to average readings
static volatile uint16_t samples[WINDOW_SIZE];
static volatile uint32_t sum;
static volatile uint32_t idx;
static struct repeating_timer timer;

bool on_sample(struct repeating_timer *t){
    // store raw reading
    uint16_t raw_adc = adc_read(); // 0 to 4095

    // moving average:
    // drop the slot we're about to overwrite (oldest value in window)
    sum -= samples[idx];
    // add new reading to sum and samples window
    sum += raw_adc;
    samples[idx] = raw_adc;
    idx = (idx+1)%WINDOW_SIZE; // wrap back to 0 after 15
    return true;
}

int main()
{
    stdio_init_all();
    cyw43_arch_init();
    
    // initialize adc sensor, input 4 is RP2040's internal temp sensor
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    int count = 0; // seconds since boot (incremented each loop)

    // start the ADC sampling timer (ISR is on_sample), runs every 100ms
    add_repeating_timer_ms(-100, on_sample, NULL, &timer);

    // turn LED on to indicate firmware running and 
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    // idle loop to print avg temp at 1Hz
    while (true) {
        
        // average adc sensor readings, convert to volts, then temp (deg C)
        printf("Time: %ds", count++);
        float avg_adc = sum / (float)WINDOW_SIZE;
        float avg_volts = avg_adc * 3.3f / 4096;
        float avg_temp = 27.0f - (avg_volts-0.706f)/0.001721f;
        printf(", Avg Temp: %.2f C\n", avg_temp);
        
        // 1Hz print cadence
        sleep_ms(1000);
    }

}
