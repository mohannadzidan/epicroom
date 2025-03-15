#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include <string>
#include <cstring>
#include <cstdio>

#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WS_OPCODE_CONTINUE 0x0
#define WS_OPCODE_TEXT_FRAME 0x1
#define WS_OPCODE_BINARY_FRAME 0x2
#define WS_OPCODE_CLOSE 0x8
#define WS_OPCODE_PING 0x9
#define WS_OPCODE_PONG 0xA

struct TCPConnection;
struct __attribute__((packed)) WSHeaderBasic
{
    u8_t opcode : 4;
    u8_t rsv : 3;
    u8_t fin : 1;
    u8_t length : 7;
    u8_t mask : 1;
};

struct __attribute__((packed)) WSHeaderExt16 : WSHeaderBasic
{
    u16_t extended_length;
};

struct __attribute__((packed)) WSHeaderExt64 : WSHeaderBasic
{
    u64_t extended_length;
};

union __attribute__((packed)) WSHeader
{
    WSHeaderBasic basic;
    WSHeaderExt16 ext16;
    WSHeaderExt64 ext64;
};

struct WSMaskedPayload
{
    u32_t masking_key;
};

class WebSocket
{
public:
    WebSocket();

    static bool handle_handshake(struct TCPConnection *connection, struct tcp_pcb *tpcb, const char *data, size_t len);
    static void handle_frame(struct TCPConnection *connection, struct tcp_pcb *tpcb, const uint8_t *data, size_t len);
    static void send_frame(struct TCPConnection *connection, const uint8_t *data, size_t len, uint8_t opcode);
};

#endif // WEBSOCKET_H