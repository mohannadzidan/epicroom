#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "TCPServer.h"
#include "WebSocket.h"
#include "log.h"
// #include <malloc.h>

void run_tcp_server()
{
    TCPServer tcpServer;
    WebSocket webSocket;
    if (!tcpServer.open(80))
    {
        log_e("Failed to open TCP server\n");
        return;
    }
    while (true)
    {

#if PICO_CYW43_ARCH_POLL
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
#else
        sleep_ms(1000);
        for (int i = 0; i < MEMP_NUM_TCP_PCB; i++)
        {
            if (tcpServer.connections[i].client_pcb != NULL && tcpServer.connections[i].is_websocket)
            {
                WSConnection *ws = reinterpret_cast<WSConnection *>(&tcpServer.connections[i]);
                ws->ping();
                // ws->write_frame(WS_OPCODE_TEXT_FRAME, 13, (const uint8_t *)"Hello, World!");
            }
        }
#endif
    }
}

// uint32_t getTotalHeap(void) {
//    extern char __StackLimit, __bss_end__;

//    return &__StackLimit  - &__bss_end__;
// }

// uint32_t getFreeHeap(void) {
//    struct mallinfo m = mallinfo();

//    return getTotalHeap() - m.uordblks;
// }

int main()
{
    stdio_init_all();

    if (cyw43_arch_init())
    {
        log_e("Failed to initialize cyw43\n");
        return 1;
    }

    uint8_t mac[6];
    cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, mac);
    log_i("Wi-Fi MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    // while (!stdio_usb_connected())
    // {
    //     sleep_ms(100);
    //     /* code */
    // }

    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        log_e("Failed to connect to wifi ssid=" WIFI_SSID " using WPA2 pass=%c%c%c****\n", WIFI_PASSWORD[0], WIFI_PASSWORD[1], WIFI_PASSWORD[2]);
        while (true)
            ;
        return 1;
    }

    run_tcp_server();
    cyw43_arch_deinit();
    return 0;
}