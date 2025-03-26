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

const char *opcode_name(uint8_t opcode)
{
    return (opcode == WS_OPCODE_TEXT_FRAME     ? "text"
            : opcode == WS_OPCODE_BINARY_FRAME ? "binary"
            : opcode == WS_OPCODE_PING         ? "ping"
            : opcode == WS_OPCODE_PONG         ? "pong"
            : opcode == WS_OPCODE_CLOSE        ? "close"
            : opcode == WS_OPCODE_CONTINUE     ? "cont"
                                               : "invalid");
}

WebSocket::WebSocket(TCPConnection *connection)
{
    this->connection = connection;
}

WSHeader header;

bool WebSocket::handle_handshake(struct TCPConnection *connection, HttpRequest *request)
{
    char headerValue[64];
    char *headerValueStart;
    int ret;
    if ((ret = request->header("Sec-WebSocket-Key", headerValue, sizeof(headerValue))) != APP_ERR_NONE)
    {
        log_e("handshake Sec-WebSocket-Key header not found! e=%d e!=%d", ret, !ret);
        return false;
    }
    char combined[128];
    snprintf(combined, sizeof(combined), "%s%s", headerValue, WS_GUID);

    uint8_t sha1[20];
    mbedtls_sha1((const unsigned char *)combined, strlen(combined), sha1);

    char accept_key[64];
    size_t accept_len = base64_encode(sha1, sizeof(sha1), accept_key, sizeof(accept_key));
    if (accept_len == 0)
    {
        // Failed to encode WebSocket accept key
        log_e("handshake Failed to encode WebSocket accept key");
        return false;
    }

    // char response[256];
    HttpResponse response(connection);
    response.status(101);
    response.header("Upgrade", "websocket");
    response.header("Connection", "Upgrade");
    response.header("Sec-WebSocket-Accept", accept_key);
    response.send();
    connection->ws = new WebSocket(connection);
    connection->timeout = 2 * 60 * 60 * 1000;
    return true;
}

void WebSocket::handle()
{

    if (connection->server->recv_len < WS_HEADER_SIZE)
    {
        log_e("WebSocket::handle received Invalid WebSocket frame");
        return;
    }
    auto header = reinterpret_cast<WSHeader *>(connection->server->buffer_recv);
    if (connection->server->recv_len < header->size())
    {
        log_e("WebSocket::handle received malformed ws");
        return;
    }
    size_t payload_len = header->getPayloadLength();
    u8_t *payload = (u8_t *)&connection->server->buffer_recv[header->size()];
    u32_t masking_key_offset = reinterpret_cast<uint8_t *>(&header->basic.masking_key) - reinterpret_cast<uint8_t *>(header);

    log_t_rx(opcode_name(header->basic.opcode),
             "src=%s:%d len=%u",
             ip4addr_ntoa(&connection->client_pcb->remote_ip),
             connection->client_pcb->remote_port,
             connection->server->recv_len);

    switch (header->basic.opcode)
    {
    case WS_OPCODE_CLOSE:
        closed = 1;
        connection->close();
        return;
    case WS_OPCODE_PING:
        pong();
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
        log_t_rx_cont("data", "string=\"%.*s\"", payload_len, payload);
    }
    else if (header->basic.opcode == WS_OPCODE_BINARY_FRAME)
    {
        log_t_rx_cont("data", "binary");
    }
#endif
}

void WebSocket::apply_mask(uint8_t *payload, size_t payload_len, u32_t maskingKey)
{
    const auto mask_bytes = reinterpret_cast<const u8_t *>(&maskingKey);
    for (size_t i = 0; i < payload_len; i++)
    {
        payload[i] ^= mask_bytes[i % 4];
    }
}

void WebSocket::write_frame(uint8_t opcode, size_t len, const uint8_t *data)
{
    write_frame_header(opcode, len);
    if (header.getPayloadLength() > 0)
        connection->write(data, len);
}

void WebSocket::write_frame_header(uint8_t opcode, size_t len)
{
    header.basic.fin = 1;
    header.basic.rsv = 0;
    header.basic.opcode = opcode;
    header.basic.mask = 0;
    header.setPayloadLength(len);
    log_t_tx(opcode_name(opcode),
             "dst=%s:%d len=%d",
             ip4addr_ntoa(&connection->client_pcb->remote_ip),
             connection->client_pcb->remote_port,
             len);
    connection->write(&header, header.size(), TCP_WRITE_FLAG_COPY | (header.getPayloadLength() > 0 ? TCP_WRITE_FLAG_MORE : 0));
}

void WebSocket::ping()
{
    write_frame(WS_OPCODE_PING, 0, NULL);
}

void WebSocket::pong()
{
    write_frame(WS_OPCODE_PONG, 0, NULL);
}

void WebSocket::writeCloseFrame()
{
    if (!closed)
    {
        write_frame(WS_OPCODE_CLOSE, 0, NULL);
        closed = true;
    }
}