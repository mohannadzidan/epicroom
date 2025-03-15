#pragma once

#include "lwip/arch.h"

struct TCPConnection;

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
    size_t getPayloadLength();
    /**
     * @brief Set the Masking key
     * 
     */
    void setMaskingKey(u32_t key);
    /**
     * @brief returns the size of the header
     * 
     */
    u8_t size();
};
