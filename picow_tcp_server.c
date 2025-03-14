#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/altcp.h"
#include "utils.h"

#define ERROR_CODE 0x4000
#define TCP_PORT 80
#define DEBUG_printf printf
#define BUF_SIZE 2048
#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
// WebSocket opcodes
#define WS_OPCODE_TEXT 0x1
#define WS_OPCODE_CLOSE 0x8

// Structure to hold the state of the TCP server
typedef struct TCP_SERVER_T_
{
    struct tcp_pcb *server_pcb;
    struct tcp_pcb *client_pcb;
    bool is_websocket;
    uint8_t buffer_recv[BUF_SIZE];
    int recv_len;
} TCP_SERVER_T;

/**
 * Initialize the TCP server state.
 * @return Pointer to the allocated TCP server state, or NULL on failure.
 */
static TCP_SERVER_T *tcp_server_init(void)
{
    TCP_SERVER_T *state = calloc(1, sizeof(TCP_SERVER_T));
    if (!state)
    {
        DEBUG_printf("Failed to allocate state\n");
        return NULL;
    }
    return state;
}

/**
 * Close the client connection and clean up resources.
 * @param arg Pointer to the TCP server state.
 * @return Error code (ERR_OK on success).
 */
static err_t tcp_server_close_client(void *arg)
{
    TCP_SERVER_T *state = (TCP_SERVER_T *)arg;
    err_t err = ERR_OK;

    if (state->client_pcb != NULL)
    {
        tcp_arg(state->client_pcb, NULL);
        tcp_poll(state->client_pcb, NULL, 0);
        tcp_sent(state->client_pcb, NULL);
        tcp_recv(state->client_pcb, NULL);
        tcp_err(state->client_pcb, NULL);
        state->is_websocket = false;
        err = tcp_close(state->client_pcb);
        if (err != ERR_OK)
        {
            DEBUG_printf("Close failed %d, calling abort\n", err);
            tcp_abort(state->client_pcb);
            err = ERR_ABRT;
        }
        state->client_pcb = NULL;
    }
    return err;
}

/**
 * Write data to the client.
 * @param arg Pointer to the TCP server state.
 * @param tpcb Client PCB.
 * @param data Pointer to the data to send.
 * @param len Length of the data.
 * @return Error code (ERR_OK on success).
 */
err_t tcp_server_write_data(void *arg, struct tcp_pcb *tpcb, const void *data, size_t len)
{
    cyw43_arch_lwip_check(); // Ensure lwIP is initialized
    err_t err = tcp_write(tpcb, data, len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK)
    {
        DEBUG_printf("Failed to write data %d\n", err);
    }
    return err;
}

/**
 * Handle WebSocket handshake.
 * @param state Pointer to the TCP server state.
 * @param tpcb Client PCB.
 * @param data Pointer to the received data.
 * @param len Length of the received data.
 * @return True if the handshake is successful, false otherwise.
 */
bool handle_websocket_handshake(TCP_SERVER_T *state, struct tcp_pcb *tpcb, const char *data, size_t len)
{
    const char *key_header = "Sec-WebSocket-Key: ";
    const char *key_start = strstr(data, key_header);
    if (!key_start)
    {
        DEBUG_printf("No WebSocket key found\n");
        return false;
    }

    key_start += strlen(key_header);
    const char *key_end = strstr(key_start, "\r\n");
    if (!key_end)
    {
        DEBUG_printf("Invalid WebSocket key format\n");
        return false;
    }

    char key[64];
    size_t key_len = key_end - key_start;
    if (key_len >= sizeof(key))
    {
        DEBUG_printf("WebSocket key too long\n");
        return false;
    }
    memcpy(key, key_start, key_len);
    key[key_len] = '\0';

    // Concatenate key with GUID and compute SHA-1 hash
    char combined[128];
    snprintf(combined, sizeof(combined), "%s%s", key, WS_GUID);

    uint8_t sha1[20];
    mbedtls_sha1((const unsigned char *)combined, strlen(combined), sha1);

    // Base64 encode the SHA-1 hash
    char accept_key[64];
    size_t accept_len = base64_encode(sha1, sizeof(sha1), accept_key, sizeof(accept_key));
    if (accept_len == 0)
    {
        DEBUG_printf("Failed to encode WebSocket accept key\n");
        return false;
    }

    // Send WebSocket handshake response
    char response[256];
    snprintf(response, sizeof(response),
             "HTTP/1.1 101 Switching Protocols\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Accept: %s\r\n\r\n",
             accept_key);
    tcp_server_write_data(state, tpcb, response, strlen(response));
    state->is_websocket = true;
    DEBUG_printf("WebSocket handshake completed\n");
    return true;
}

/**
 * Handle WebSocket frame.
 * @param state Pointer to the TCP server state.
 * @param tpcb Client PCB.
 * @param data Pointer to the received frame.
 * @param len Length of the received frame.
 */
void handle_websocket_frame(TCP_SERVER_T *state, struct tcp_pcb *tpcb, const uint8_t *data, size_t len)
{
    if (len < 2)
    {
        DEBUG_printf("Invalid WebSocket frame\n");
        return;
    }

    uint8_t opcode = data[0] & 0x0F;
    bool is_masked = (data[1] & 0x80) != 0;
    uint64_t payload_len = data[1] & 0x7F;

    size_t header_len = 2;
    if (payload_len == 126)
    {
        if (len < 4)
        {
            DEBUG_printf("Invalid WebSocket frame (126)\n");
            return;
        }
        payload_len = (data[2] << 8) | data[3];
        header_len += 2;
    }
    else if (payload_len == 127)
    {
        if (len < 10)
        {
            DEBUG_printf("Invalid WebSocket frame (127)\n");
            return;
        }
        payload_len = ((uint64_t)data[2] << 56) | ((uint64_t)data[3] << 48) |
                      ((uint64_t)data[4] << 40) | ((uint64_t)data[5] << 32) |
                      ((uint64_t)data[6] << 24) | ((uint64_t)data[7] << 16) |
                      ((uint64_t)data[8] << 8) | data[9];
        header_len += 8;
    }

    if (is_masked)
    {
        if (len < header_len + 4)
        {
            DEBUG_printf("Invalid WebSocket frame (mask)\n");
            return;
        }
        const uint8_t *mask = &data[header_len];
        header_len += 4;

        // Unmask the payload
        uint8_t *payload = (uint8_t *)&data[header_len];
        for (size_t i = 0; i < payload_len; i++)
        {
            payload[i] ^= mask[i % 4];
        }
    }

    if (opcode == WS_OPCODE_TEXT)
    {
        DEBUG_printf("Received WebSocket text frame: %.*s\n", (int)payload_len, &data[header_len]);
        // Echo the message back to the client
        tcp_server_write_data(state, tpcb, &data[header_len], payload_len);
    }
    else if (opcode == WS_OPCODE_CLOSE)
    {
        DEBUG_printf("WebSocket connection closed by client\n");
        tcp_server_close_client(state);
    }
}

/**
 * Handle received data from the client.
 * @param arg Pointer to the TCP server state.
 * @param tpcb Client PCB.
 * @param p Pointer to the received packet buffer.
 * @param err Error code.
 * @return Error code (ERR_OK on success).
 */
err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    TCP_SERVER_T *state = (TCP_SERVER_T *)arg;

    DEBUG_printf("===state===\r\n"
                 "server_pcb=%p\r\n"
                 "client_pcb=%p\r\n"
                 "is_websocket=%s\r\n"
                 "============\r\n",
                 state->server_pcb,
                 state->client_pcb,
                 state->is_websocket ? "true" : "false");
    if (!p)
    {
        // Client closed the connection
        DEBUG_printf("Client closed the connection\n");
        tcp_server_close_client(arg);
        return ERR_OK;
    }

    cyw43_arch_lwip_check(); // Ensure lwIP is initialized

    // Copy received data into the buffer
    state->recv_len = pbuf_copy_partial(p, state->buffer_recv, p->tot_len, 0);
    state->buffer_recv[state->recv_len] = '\0'; // Null-terminate the received data
    tcp_recved(tpcb, p->tot_len);               // Acknowledge receipt of data
    pbuf_free(p);                               // Free the packet buffer

    if (!state->is_websocket)
    {
        // Handle WebSocket handshake
        if (!handle_websocket_handshake(state, tpcb, (const char *)state->buffer_recv, state->recv_len))
        {
            DEBUG_printf("WebSocket handshake failed\n");
            char response[32];
            snprintf(response, sizeof(response),
                     "HTTP/1.1 418 I'm a teapot\r\n");
            tcp_server_write_data(state, tpcb, response, sizeof(response));
            tcp_server_close_client(state);
        }

        // tcp_server_close_client(arg);
    }
    else
    {
        // Handle WebSocket frame
        handle_websocket_frame(state, tpcb, state->buffer_recv, state->recv_len);
    }

    return ERR_OK;
}

// Rest of the code remains the same...

/**
 * Handle client connection acceptance.
 * @param arg Pointer to the TCP server state.
 * @param client_pcb Client PCB.
 * @param err Error code.
 * @return Error code (ERR_OK on success).
 */
static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err)
{
    TCP_SERVER_T *state = (TCP_SERVER_T *)arg;

    if (err != ERR_OK || client_pcb == NULL)
    {
        DEBUG_printf("Failure in accept\n");
        return ERR_VAL;
    }

    DEBUG_printf("Client connected\n");

    // Set up the client PCB
    state->client_pcb = client_pcb;
    tcp_arg(client_pcb, state);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_err(client_pcb, NULL);

    return ERR_OK;
}

/**
 * Open the TCP server and start listening for connections.
 * @param arg Pointer to the TCP server state.
 * @return True on success, false on failure.
 */
static bool tcp_server_open(void *arg)
{
    TCP_SERVER_T *state = (TCP_SERVER_T *)arg;

    DEBUG_printf("Starting server on port %u\n", TCP_PORT);

    // Create a new TCP PCB
    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb)
    {
        DEBUG_printf("Failed to create PCB\n");
        return false;
    }

    // Bind the PCB to the specified port
    err_t err = tcp_bind(pcb, NULL, TCP_PORT);
    if (err)
    {
        DEBUG_printf("Failed to bind to port %u\n", TCP_PORT);
        return false;
    }

    // Start listening for connections
    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb)
    {
        DEBUG_printf("Failed to listen\n");
        tcp_close(pcb);
        return false;
    }

    // Set up the accept callback
    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, tcp_server_accept);

    return true;
}

/**
 * Main function to run the TCP server.
 */
void run_tcp_server(void)
{
    TCP_SERVER_T *state = tcp_server_init();
    if (!state)
    {
        return;
    }

    if (!tcp_server_open(state))
    {
        DEBUG_printf("Failed to open TCP server\n");
        free(state);
        return;
    }

    while (true)
    {
        // Poll for Wi-Fi and lwIP work
#if PICO_CYW43_ARCH_POLL
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
#else
        sleep_ms(1000);
#endif
    }

    free(state);
}

int main()
{
    stdio_init_all();

    if (cyw43_arch_init())
    {
        printf("Failed to initialize\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        printf("Failed to connect\n");
        return 1;
    }
    else
    {
        printf("Connected\n");
    }
    run_tcp_server();
    cyw43_arch_deinit();
    return 0;
}