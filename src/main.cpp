#include "TCPServer.h"
#include "WebSocket.h"

void run_tcp_server()
{
    TCPServer tcpServer;
    WebSocket webSocket;
    printf("starting server \n");

    if (!tcpServer.open(80))
    {
        printf("Failed to open TCP server\n");
        return;
    }
    else
    {
        printf("TCP server opened at port 80\n");
    }
    printf("starting server loop\n");

    while (true)
    {

#if PICO_CYW43_ARCH_POLL
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
#else
        sleep_ms(1000);
        for (int i = 0; i < MEMP_NUM_TCP_PCB; i++)
        {
            if (tcpServer.connections[i].client_pcb != NULL)
            {
                WebSocket::send_frame(&tcpServer.connections[i], (const uint8_t *)"Hello, World!", 13, WS_OPCODE_TEXT_FRAME);
                printf("Sent message to client(%d)\n", i);
            }
        }
#endif
    }
}

int main()
{
    stdio_init_all();

    if (cyw43_arch_init())
    {
        printf("Failed to initialize\n");
        return 1;
    }

    // while (!stdio_usb_connected())
    // {
    //     sleep_ms(100);
    //     /* code */
    // }

    cyw43_arch_enable_sta_mode();
    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        printf("Failed to connect to wifi ssid=" WIFI_SSID " using WPA2 pass=%c%c%c****\n", WIFI_PASSWORD[0], WIFI_PASSWORD[1], WIFI_PASSWORD[2]);
        return 1;
    }
    else
    {
        uint8_t mac[6];
        cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, mac);
        printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    run_tcp_server();
    cyw43_arch_deinit();
    return 0;
}