#include <stdio.h>
#include "WebSocket.h"
#include "WSHeader.h"
#include "TCPServer.h"
#include "utils.h"


WebSocket::WebSocket() {}

WSHeader header;

bool WebSocket::handle_handshake(struct TCPConnection *connection, struct tcp_pcb *tpcb, const char *data, size_t len)
{
    printf("connection is %p\n", connection);
    const char *key_header = "Sec-WebSocket-Key: ";
    const char *key_start = strstr(data, key_header);
    if (!key_start)
    {
        printf("No WebSocket key found\n");
        return false;
    }

    key_start += strlen(key_header);
    const char *key_end = strstr(key_start, "\r\n");
    if (!key_end)
    {
        printf("Invalid WebSocket key format\n");
        return false;
    }

    char key[64];
    size_t key_len = key_end - key_start;
    if (key_len >= sizeof(key))
    {
        printf("WebSocket key too long\n");
        return false;
    }
    memcpy(key, key_start, key_len);
    key[key_len] = '\0';

    char combined[128];
    snprintf(combined, sizeof(combined), "%s%s", key, WS_GUID);

    uint8_t sha1[20];
    mbedtls_sha1((const unsigned char *)combined, strlen(combined), sha1);

    char accept_key[64];
    size_t accept_len = base64_encode(sha1, sizeof(sha1), accept_key, sizeof(accept_key));
    if (accept_len == 0)
    {
        printf("Failed to encode WebSocket accept key\n");
        return false;
    }

    char response[256];
    snprintf(response, sizeof(response),
             "HTTP/1.1 101 Switching Protocols\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Accept: %s\r\n\r\n",
             accept_key);
    connection->write(response, strlen(response));
    connection->is_websocket = true;
    printf("WebSocket handshake completed\n");
    return true;
}

void WebSocket::handle_frame(struct TCPConnection *connection, struct tcp_pcb *tpcb, const uint8_t *data, size_t len)
{
    if (len < 2)
    {
        printf("Invalid WebSocket frame\n");
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
            printf("Invalid WebSocket frame (126)\n");
            return;
        }
        payload_len = (data[2] << 8) | data[3];
        header_len += 2;
    }
    else if (payload_len == 127)
    {
        if (len < 10)
        {
            printf("Invalid WebSocket frame (127)\n");
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
            printf("Invalid WebSocket frame (mask)\n");
            return;
        }
        const uint8_t *mask = &data[header_len];
        header_len += 4;

        uint8_t *payload = (uint8_t *)&data[header_len];
        for (size_t i = 0; i < payload_len; i++)
        {
            payload[i] ^= mask[i % 4];
        }
    }

    if (opcode == WS_OPCODE_TEXT_FRAME)
    {
        printf("Received WebSocket text frame: %.*s\n", (int)payload_len, &data[header_len]);
        connection->write(&data[header_len], payload_len);
    }
    else if (opcode == WS_OPCODE_CLOSE)
    {
        printf("WebSocket connection closed by client\n");
        connection->close();
    }
}

u8_t getWSHeaderSize()
{
    u8_t size = 2;
    if (header.basic.length == 126)
    {
        size += 2;
    }
    else if (header.basic.length == 127)
    {
        size += 8;
    }
    if (header.basic.mask)
    {
        size += 4;
    }
    return size;
}

void WebSocket::send_frame(struct TCPConnection *connection, const uint8_t *data, size_t len, uint8_t opcode)
{
    header.basic.fin = 1;
    header.basic.rsv = 0;
    header.basic.opcode = opcode;
    header.basic.mask = 0;
    header.setPayloadLength(len);
    connection->write(&header, header.size(), TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE);
    connection->write(data, len);
}
