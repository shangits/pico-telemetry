#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
#include "wifi_config.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
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
    // sleep for 6s to provide time to attach screen before outputs start
    sleep_ms(6000);

    // initialize with country set to Canada and join as a client
    cyw43_arch_init_with_country(CYW43_COUNTRY_CANADA);
    cyw43_arch_enable_sta_mode();
    // note: put your own personal wifi details in a wifi_config.h file
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID,WIFI_PWD,CYW43_AUTH_WPA2_AES_PSK, 30000) == 0) {
        printf(
            "Connected! IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_default))
        );
    } else {
        printf("Wi-Fi connection failed\n");
    }

    
    // initialize adc sensor, input 4 is RP2040's internal temp sensor
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    int count = 0; // seconds since boot (incremented each loop)

    // start the ADC sampling timer (ISR is on_sample), runs every 100ms
    add_repeating_timer_ms(-100, on_sample, NULL, &timer);

    // turn LED on to indicate firmware running
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
