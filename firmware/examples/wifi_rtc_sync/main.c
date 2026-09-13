// Connects to WiFi, fetches the current time over NTP, and writes it to
// the DS3231. NTP request/response handling is adapted from the pico-sdk
// pico-examples project (pico_w/wifi/ntp_client/picow_ntp_client.c).
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "hardware/i2c.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"

#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

#include "ds3231.h"

// Same confirmed, hardware-correct I2C1 pins as the rtc_time/power_monitor
// examples -- see firmware/README.md.
#define RTC_I2C_PORT i2c1
#define RTC_SDA_PIN 38
#define RTC_SCL_PIN 39
#define RTC_I2C_BAUDRATE (100 * 1000)

#define NTP_SERVER "pool.ntp.org"
#define NTP_MSG_LEN 48
#define NTP_PORT 123
#define NTP_DELTA 2208988800u // seconds between 1 Jan 1900 and 1 Jan 1970
#define NTP_RESEND_TIME_MS (10 * 1000)
#define NTP_TIMEOUT_MS (30 * 1000)

typedef struct {
    ip_addr_t ntp_server_address;
    struct udp_pcb *ntp_pcb;
    async_at_time_worker_t resend_worker;
    ds3231_t *rtc;
    volatile bool done;
    volatile bool ok;
} ntp_state_t;

static void ntp_result(ntp_state_t *state, bool ok, const time_t *result) {
    async_context_remove_at_time_worker(cyw43_arch_async_context(), &state->resend_worker);
    if (ok && result) {
        struct tm utc;
        gmtime_r(result, &utc);
        datetime_t dt;
        tm_to_datetime(&utc, &dt);
        if (ds3231_set_datetime(state->rtc, &dt)) {
            char text[32];
            datetime_to_str(text, sizeof(text), &dt);
            printf("NTP time %s (UTC) written to the RTC\n", text);
        } else {
            printf("got NTP time but failed to write it to the RTC\n");
            ok = false;
        }
    } else {
        printf("NTP request failed\n");
    }
    state->ok = ok;
    state->done = true;
}

static void ntp_request(ntp_state_t *state) {
    // cyw43_arch_lwip_begin/end are no-ops in poll mode, but are required
    // around lwIP calls made outside of an lwIP callback when using
    // threadsafe_background mode.
    cyw43_arch_lwip_begin();
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
    uint8_t *req = (uint8_t *)p->payload;
    memset(req, 0, NTP_MSG_LEN);
    req[0] = 0x1b;
    udp_sendto(state->ntp_pcb, p, &state->ntp_server_address, NTP_PORT);
    pbuf_free(p);
    cyw43_arch_lwip_end();
}

static void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    ntp_state_t *state = (ntp_state_t *)arg;
    if (ipaddr) {
        state->ntp_server_address = *ipaddr;
        printf("ntp address %s\n", ipaddr_ntoa(ipaddr));
        ntp_request(state);
    } else {
        printf("ntp dns request failed\n");
        ntp_result(state, false, NULL);
    }
}

static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    ntp_state_t *state = (ntp_state_t *)arg;
    uint8_t mode = pbuf_get_at(p, 0) & 0x7;
    uint8_t stratum = pbuf_get_at(p, 1);

    if (ip_addr_cmp(addr, &state->ntp_server_address) && port == NTP_PORT && p->tot_len == NTP_MSG_LEN &&
        mode == 0x4 && stratum != 0) {
        uint8_t seconds_buf[4] = {0};
        pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);
        uint32_t seconds_since_1900 = ((uint32_t)seconds_buf[0] << 24) | ((uint32_t)seconds_buf[1] << 16) |
                                       ((uint32_t)seconds_buf[2] << 8) | seconds_buf[3];
        time_t epoch = (time_t)(seconds_since_1900 - NTP_DELTA);
        ntp_result(state, true, &epoch);
    } else {
        printf("invalid ntp response\n");
        ntp_result(state, false, NULL);
    }
    pbuf_free(p);
}

static void resend_worker_fn(__unused async_context_t *context, async_at_time_worker_t *worker) {
    ntp_state_t *state = (ntp_state_t *)worker->user_data;
    printf("ntp request timed out\n");
    ntp_result(state, false, NULL);
}

// Fetches the current time over NTP once and writes it to the RTC. Returns
// once that either succeeds, fails, or times out.
static bool sync_rtc_from_ntp(ds3231_t *rtc) {
    ntp_state_t state = {0};
    state.rtc = rtc;
    state.ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!state.ntp_pcb) {
        printf("failed to create UDP PCB\n");
        return false;
    }
    udp_recv(state.ntp_pcb, ntp_recv, &state);
    state.resend_worker.do_work = resend_worker_fn;
    state.resend_worker.user_data = &state;

    hard_assert(async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &state.resend_worker,
                                                        NTP_RESEND_TIME_MS));

    int err = dns_gethostbyname(NTP_SERVER, &state.ntp_server_address, ntp_dns_found, &state);
    if (err == ERR_OK) {
        ntp_request(&state); // cached DNS result, request immediately
    } else if (err != ERR_INPROGRESS) {
        printf("dns request failed\n");
        ntp_result(&state, false, NULL);
    }

    absolute_time_t deadline = make_timeout_time_ms(NTP_TIMEOUT_MS);
    while (!state.done && !time_reached(deadline)) {
#if PICO_CYW43_ARCH_POLL
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(100));
#else
        sleep_ms(100);
#endif
    }

    udp_remove(state.ntp_pcb);
    return state.done && state.ok;
}

int main(void) {
    stdio_init_all();

    i2c_init(RTC_I2C_PORT, RTC_I2C_BAUDRATE);
    gpio_set_function(RTC_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(RTC_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(RTC_SDA_PIN);
    gpio_pull_up(RTC_SCL_PIN);

    ds3231_t rtc;
    if (!ds3231_init(&rtc, RTC_I2C_PORT)) {
        printf("DS3231 not responding on the bus\n");
        return 1;
    }

    if (cyw43_arch_init()) {
        printf("failed to initialise wifi hardware\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();

    printf("connecting to %s...\n", WIFI_SSID);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect to wifi\n");
        return 1;
    }
    printf("connected\n");

    if (!sync_rtc_from_ntp(&rtc)) {
        printf("could not sync RTC from NTP\n");
    }

    cyw43_arch_disable_sta_mode();

    // Demonstrate the RTC is now actually ticking with the synced time.
    while (true) {
        datetime_t now;
        if (ds3231_get_datetime(&rtc, &now)) {
            char text[32];
            datetime_to_str(text, sizeof(text), &now);
            printf("%s\n", text);
        }
        sleep_ms(1000);
    }
}
