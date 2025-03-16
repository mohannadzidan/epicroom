
#include "WSHeader.h"

size_t WSHeader::getPayloadLength() const
{
    if (basic.length == 126)
    {
        return ext16.extended_length << 7 + ext16.length;
    }
    else if (basic.length == 127)
    {
        return ext64.extended_length << 7 + ext64.length;
    }
    return basic.length;
}

void WSHeader::setPayloadLength(size_t len)
{
    if (len > 127)
    {
        basic.length = 127;
        ext64.extended_length = len >> 7;
    }
    else if (len == 126)
    {
        basic.length = 126;
        ext16.extended_length = len >> 7;
    }
    else
    {
        basic.length = len;
    }
}

void WSHeader::setMaskingKey(u32_t key)
{
    if (basic.length > 127)
    {
        ext64.masking_key = key;
    }
    else if (basic.length == 126)
    {
        ext16.masking_key = key;
    }
    else
    {
        basic.masking_key = key;
    }
}

u32_t WSHeader::getMaskingKey() const
{
    if (basic.length > 127)
    {
        return ext64.masking_key;
    }
    else if (basic.length == 126)
    {
        return ext16.masking_key;
    }
    else
    {
        return basic.masking_key;
    }
}

u8_t WSHeader::size() const
{
    u8_t size = WS_HEADER_SIZE;
    if (basic.length == 126)
    {
        size = WS_HEADER_EXT16_SIZE;
    }
    else if (basic.length == 127)
    {
        size = WS_HEADER_EXT64_SIZE;
    }
    if (basic.mask)
    {
        size += WS_HEADER_MASKING_KEY_SIZE;
    }
    return size;
}