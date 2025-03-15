#pragma once

#include "lwip/arch.h"


#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WS_OPCODE_CONTINUE 0x0
#define WS_OPCODE_TEXT_FRAME 0x1
#define WS_OPCODE_BINARY_FRAME 0x2
#define WS_OPCODE_CLOSE 0x8
#define WS_OPCODE_PING 0x9
#define WS_OPCODE_PONG 0xA

struct TCPConnection;
class WebSocket
{
public:
    WebSocket();

    static bool handle_handshake(struct TCPConnection *connection, struct tcp_pcb *tpcb, const char *data, size_t len);
    static void handle_frame(struct TCPConnection *connection, struct tcp_pcb *tpcb, const uint8_t *data, size_t len);
    static void send_frame(struct TCPConnection *connection, const uint8_t *data, size_t len, uint8_t opcode);
};
