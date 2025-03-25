#pragma once

#include "lwip/arch.h"
#include "TCPServer.h"
#include "TCPConnection.h"
#include "http.h"
#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WS_OPCODE_CONTINUE 0x0
#define WS_OPCODE_TEXT_FRAME 0x1
#define WS_OPCODE_BINARY_FRAME 0x2
#define WS_OPCODE_CLOSE 0x8
#define WS_OPCODE_PING 0x9
#define WS_OPCODE_PONG 0xA

struct TCPConnection;
struct HttpRequest;
class WebSocket
{
public:
    WebSocket(TCPConnection *connection);
    TCPConnection *connection;
    u8_t closed : 1;
    void write_frame(uint8_t opcode, size_t len, const uint8_t *data);
    void write_frame_header(uint8_t opcode, size_t len);
    void ping();
    void pong();
    void close();
    
    void handle();
    static void apply_mask(uint8_t *payload, size_t payload_len, u32_t maskingKey);
    static bool handle_handshake(struct TCPConnection *connection, HttpRequest *request);
    static void handle_frame(struct TCPConnection *connection, struct tcp_pcb *tpcb, uint8_t *data, size_t len);
};
