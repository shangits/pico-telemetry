#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
#include "wifi_config.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/tcp.h"
#define WINDOW_SIZE 16

// initializing variables for ring buffer to average readings
static volatile uint16_t samples[WINDOW_SIZE];
static volatile uint32_t sum;
static volatile uint32_t idx;
static struct repeating_timer timer;

// returns the current moving average as degrees C
// ISR keeps 'sum' updated, conversion math is processed here
static float read_temperature_c(void)
{
    float avg_adc = sum / (float)WINDOW_SIZE;
    float avg_volts = avg_adc * 3.3f / 4096;
    float avg_temp = 27.0f - ((avg_volts - 0.706f) / 0.001721f);
    return avg_temp;
}

bool on_sample(struct repeating_timer *t)
{

    // read the adc voltage directly from the registers instead of adc_read()
    // set START_ONCE (bit 2 of CS as per RP2040 datasheet) bit to 1
    adc_hw->cs |= ADC_CS_START_ONCE_BITS; // starts one conversion

    // wait for conversion to complete, READY (bit 8) goes back to 1
    while (!(adc_hw->cs & ADC_CS_READY_BITS))
    {
        tight_loop_contents();
    }
    // grab 12 bit result, 0 to 4095
    uint16_t raw_adc = adc_hw->result;

    // moving average:
    // drop the slot we're about to overwrite (oldest value in window)
    sum -= samples[idx];
    // add new reading to sum and samples window
    sum += raw_adc;
    samples[idx] = raw_adc;
    idx = (idx + 1) % WINDOW_SIZE; // wrap back to 0 after 15
    return true;
}

// runs when the client's request arrives, replies with
//  the temp as JSON and closes
static err_t on_recv(void *arg, struct tcp_pcb *tpcb,
                     struct pbuf *p, err_t err)
{
    // p is NULL when the client closes its side, nothing to reply to
    if (p == NULL)
    {
        tcp_close(tpcb);
        return ERR_OK;
    }
    // all connection requests get the same reply
    // still have to tell lwIP we read it and free it, otherwise it fills up
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);

    // building the HTTP response
    char buf[128];
    int mes = snprintf(buf, sizeof(buf),
                       "HTTP/1.1 200 OK\r\n"                // status line
                       "Content-Type: application/json\r\n" // headers
                       "Connection: close\r\n\r\n"          // blank line to end headers
                       "{\"temp_c\": %.2f}\r\n",
                       read_temperature_c()); // JSON body

    // TCP_WRITE_FLAG_COPY tells lwIP to copy buf, so our
    //  stack buffer can go away
    tcp_write(tpcb, buf, mes, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb); // flush now
    tcp_close(tpcb);

    return ERR_OK;
}

// runs when a client connects to port 80, hands off to on_recv
static err_t on_accept(void *arg, struct tcp_pcb *client_pcb, err_t err)
{
    tcp_recv(client_pcb, on_recv);
    return ERR_OK;
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
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PWD, CYW43_AUTH_WPA2_AES_PSK, 30000) == 0)
    {
        printf(
            "Connected! IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_default)));

        // set up TCP server on port 80 (default HTTP port)
        struct tcp_pcb *server_pcb = tcp_new();
        // bind to any interface
        tcp_bind(server_pcb, IP_ANY_TYPE, 80);
        // flip to listening mode
        server_pcb = tcp_listen(server_pcb);
        // run on_accept when a client connects
        tcp_accept(server_pcb, on_accept);
    }
    else
    {
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
    while (true)
    {
        // print temp over USB at 1Hz for debugging
        printf("Time: %ds, Avg Temp: %.2f C\n", count++, read_temperature_c());
        sleep_ms(1000);
    }
}
