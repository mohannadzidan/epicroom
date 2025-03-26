#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "AppHttpServer.h"
#include "WebSocket.h"
#include "log.h"
#include <malloc.h>
#include "assets_bundle.h"
#include "hardware/watchdog.h"
#include <time.h>
#include "hardware.h"

bool rebootedByWatchdog = false;
time_t myTime(time_t *t)
{
    *t = (((2023 - 1970) * 12 + 8) * 30 * 24 * 60 * 60);
    return *t;
}

uint32_t getTotalHeap(void)
{
    extern char __StackLimit, __bss_end__;

    return &__StackLimit - &__bss_end__;
}

uint32_t getFreeHeap(void)
{
    struct mallinfo m = mallinfo();

    return getTotalHeap() - m.uordblks;
}

AppHttpServer tcpServer = AppHttpServer(true);
AppHttpServer httpServer = AppHttpServer(false);

void run_tcp_server()
{

    if (!tcpServer.open(443))
    {
        log_e("Failed to open TCP server");
        return;
    }
    if (!httpServer.open(80))
    {
        log_e("Failed to open HTTP server");
        return;
    }

    while (true)
    {

#if PICO_CYW43_ARCH_POLL
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
#else
        watchdog_update();
        log_i("[sys] heap=%0.2f%% connections=%d",
              (float)(getTotalHeap() - getFreeHeap()) / (float)getTotalHeap() * 100,
              TCPConnection::activeCount);
        sleep_ms(1000);
        ledToggle();
        TCPServer::poll();
        // for (int i = 0; i < MEMP_NUM_TCP_PCB; i++)
        // {
        //     if (tcpServer.connections[i].client_pcb != NULL && tcpServer.connections[i].ws != nullptr && !tcpServer.connections[i].ws->closed)
        //     {

        //         tcpServer.connections[i].ws->ping();
        //         tcpServer.connections[i].ws->write_frame(WS_OPCODE_TEXT_FRAME, 13, (const uint8_t *)"Hello, World!");
        //     }
        // }
#endif
    }
}

int main()
{
    stdio_init_all();
    rebootedByWatchdog = watchdog_enable_caused_reboot();

    watchdog_enable(8388, 1);
    if (cyw43_arch_init())
    {
        log_e("Failed to initialize cyw43");
        return 1;
    }

    watchdog_update();

    sleep_ms(1000);
    if (rebootedByWatchdog)
    {
        log_e("Rebooted by Watchdog!\n");
    }
    log_i("Device up ٩(◕‿◕)۶");

    // while (!stdio_usb_connected())
    // {
    //     sleep_ms(100);
    //     /* code */
    // }

    log_im("Starting WiFi ...");
    cyw43_arch_enable_sta_mode();
    printf(" OK\n");
    uint8_t mac[6];
    cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, mac);
    log_i("Wi-Fi MAC Address: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    bool connected = false;
    while (!connected)
    {

        watchdog_update();
        log_im("Connecting to wifi ...");
        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 4900))
        {
            log_e("Failed to connect to wifi ssid=" WIFI_SSID " using WPA2 pass=%.sc****", 3, WIFI_PASSWORD);
        }
        else
        {
            connected = true;
        }
    }
    watchdog_update();
    printf(" OK\n");
    ledOff();
    run_tcp_server();
    log_i("Shutdown...");
    cyw43_arch_deinit();
    log_i("Good Bye! ༼つ ◕_◕ ༽つ");
    return 0;
}