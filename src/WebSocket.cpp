#include <string.h>
#include "WebSocket.h"
#include "WSHeader.h"
#include "TCPServer.h"
#include "utils.h"
#include "log.h"

#define ARROW_TX "--ws->"
#define ARROW_RX "<-ws--"
#define ARROW_SP "      "
#define log_t_rx(tag, message, ...) log_t(ARROW_RX " %s -- " message, tag, ##__VA_ARGS__)
#define log_t_rx_cont(tag, message, ...) log_t(ARROW_SP " %s -- " message, tag, ##__VA_ARGS__)
#define log_t_tx(tag, message, ...) log_t(ARROW_TX " %s -- " message, tag, ##__VA_ARGS__)
#define log_t_tx_cont(tag, message, ...) log_t(ARROW_SP " %s -- " message, tag, ##__VA_ARGS__)

#define opcode_name(opcode) (opcode == WS_OPCODE_TEXT_FRAME     ? "text"   \
                             : opcode == WS_OPCODE_BINARY_FRAME ? "binary" \
                             : opcode == WS_OPCODE_PING         ? "ping"   \
                             : opcode == WS_OPCODE_PONG         ? "pong"   \
                             : opcode == WS_OPCODE_CLOSE        ? "close"  \
                             : opcode == WS_OPCODE_CONTINUE     ? "cont"   \
                                                                : "invalid")

WebSocket::WebSocket()
{
}

WSHeader header;

bool WebSocket::handle_handshake(struct TCPConnection *connection, struct tcp_pcb *tpcb, const char *data, size_t len)
{
    const char *key_header = "Sec-WebSocket-Key: ";
    const char *key_start = strstr(data, key_header);
    if (!key_start)
    {
        // No WebSocket key found, assume it's not a WebSocket connection
        return false;
    }

    key_start += strlen(key_header);
    const char *key_end = strstr(key_start, "\r\n");
    if (!key_end)
    {
        // Invalid WebSocket key format
        return false;
    }

    char key[64];
    size_t key_len = key_end - key_start;
    if (key_len >= sizeof(key))
    {
        // WebSocket key too long
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
        // Failed to encode WebSocket accept key
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
    return true;
}

void WebSocket::handle_frame(struct TCPConnection *connection, struct tcp_pcb *tpcb, uint8_t *data, size_t len)
{

    if (len < WS_HEADER_SIZE)
    {
        // Invalid WebSocket frame
        return;
    }
    WSHeader *header = reinterpret_cast<WSHeader *>(data);
    if (len < header->size())
    {
        // malformed frame
        return;
    }
    WSConnection *ws = reinterpret_cast<WSConnection *>(connection);
    size_t payload_len = header->getPayloadLength();
    u8_t *payload = (u8_t *)&data[header->size()];
    u32_t masking_key_offset = reinterpret_cast<uint8_t *>(&header->basic.masking_key) - reinterpret_cast<uint8_t *>(header);

    log_t_rx(opcode_name(header->basic.opcode),
             "src=%s:%d len=%u \n",
             ip4addr_ntoa(&tpcb->remote_ip),
             tpcb->remote_port,
             len);

    switch (header->basic.opcode)
    {
    case WS_OPCODE_CLOSE:
        ws->closed = 1;
        ws->close();
        return;
    case WS_OPCODE_PING:
        ws->pong();
    case WS_OPCODE_PONG:
        return;
    default:
        break;
    }

    if (header->basic.mask)
    {
        apply_mask(payload, payload_len, header->getMaskingKey());
    }

#if LOGGING_LEVEL >= LOGGING_LEVEL_TRACE
    if (header->basic.opcode == WS_OPCODE_TEXT_FRAME)
    {
        log_t_rx_cont("data", "string=\"%.*s\"\n", payload_len, payload);
    }
    else if (header->basic.opcode == WS_OPCODE_BINARY_FRAME)
    {
        log_t_rx_cont("data", "binary\n");
    }
#endif
}

void WebSocket::apply_mask(uint8_t *payload, size_t payload_len, u32_t maskingKey)
{
    const u8_t *mask_bytes = reinterpret_cast<const u8_t *>(&maskingKey);
    for (size_t i = 0; i < payload_len; i++)
    {
        payload[i] ^= mask_bytes[i % 4];
    }
}

void WSConnection::write_frame(uint8_t opcode, size_t len, const uint8_t *data)
{
    write_frame_header(opcode, len);
    if (header.getPayloadLength() > 0)
        write(data, len);
}

void WSConnection::write_frame_header(uint8_t opcode, size_t len)
{
    header.basic.fin = 1;
    header.basic.rsv = 0;
    header.basic.opcode = opcode;
    header.basic.mask = 0;
    header.setPayloadLength(len);
    log_t_tx(opcode_name(opcode),
             "dst=%s:%d len=%d \n",
             ip4addr_ntoa(&client_pcb->remote_ip),
             client_pcb->remote_port,
             len);
    write(&header, header.size(), TCP_WRITE_FLAG_COPY | (header.getPayloadLength() > 0 ? TCP_WRITE_FLAG_MORE : 0));
}

void WSConnection::ping()
{
    write_frame(WS_OPCODE_PING, 0, NULL);
}

void WSConnection::pong()
{
    write_frame(WS_OPCODE_PONG, 0, NULL);
}

void WSConnection::close()
{
    if (!closed)
    {
        write_frame(WS_OPCODE_CLOSE, 0, NULL);
        closed = true;
    }
    TCPConnection::close();
}