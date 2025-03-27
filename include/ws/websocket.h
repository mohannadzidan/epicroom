#pragma once

#include "lwip/arch.h"
#include "tcp/core.h"
#include "tcp/core.h"
#include "http/core.h"
#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WS_OPCODE_CONTINUE 0x0
#define WS_OPCODE_TEXT_FRAME 0x1
#define WS_OPCODE_BINARY_FRAME 0x2
#define WS_OPCODE_CLOSE 0x8
#define WS_OPCODE_PING 0x9
#define WS_OPCODE_PONG 0xA
#define WS_HEADER_SIZE 2
#define WS_HEADER_EXT16_SIZE 4
#define WS_HEADER_EXT64_SIZE 10
#define WS_HEADER_MASKING_KEY_SIZE 4

struct TCPConnection;
struct HttpRequest;

#pragma region WSHeader

struct __attribute__((packed)) WSHeaderEssentials
{
    u8_t opcode : 4;
    u8_t rsv : 3;
    u8_t fin : 1;
    u8_t length : 7;
    u8_t mask : 1;
};

struct __attribute__((packed)) WSHeaderBasic : WSHeaderEssentials
{
    u32_t masking_key;
};

struct __attribute__((packed)) WSHeaderExt16 : WSHeaderEssentials
{
    u16_t extended_length;
    u32_t masking_key;
};

struct __attribute__((packed)) WSHeaderExt64 : WSHeaderEssentials
{
    u64_t extended_length;
    u32_t masking_key;
};

union __attribute__((packed)) WSHeader
{
    WSHeaderBasic basic;
    WSHeaderExt16 ext16;
    WSHeaderExt64 ext64;

    /**
     * @brief Set the payload length
     *
     */
    void setPayloadLength(size_t len);
    /**
     * @brief Get the payload length
     *
     * @return int
     */
    size_t getPayloadLength() const;
    /**
     * @brief Set the Masking key
     *
     */
    void setMaskingKey(u32_t key);
    /**
     * @brief returns the size of the header
     *
     */
    u8_t size() const;

    u32_t getMaskingKey() const;
};
#pragma endregion
#pragma region WebSocket
class WebSocket
{
public:
    WebSocket(TCPConnection *connection);
    TCPConnection *connection;
    u8_t closed : 1;
    void write_frame(uint8_t opcode, size_t len, const uint8_t *data);
    void write_frame_header(uint8_t opcode, size_t len);
    void writeCloseFrame();
    void ping();
    void pong();
    
    void handle();
    static void apply_mask(uint8_t *payload, size_t payload_len, u32_t maskingKey);
    static bool handle_handshake(struct TCPConnection *connection, HttpRequest *request);
    static void handle_frame(struct TCPConnection *connection, struct tcp_pcb *tpcb, uint8_t *data, size_t len);
};

#pragma endregion
