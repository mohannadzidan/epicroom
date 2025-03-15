
#include "WSHeader.h"

size_t WSHeader::getPayloadLength()
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

u8_t WSHeader::size()
{
    u8_t size = 2;
    if (basic.length == 126)
    {
        size += 2;
       
    }
    else if (basic.length == 127)
    {
        size += 8;
    }
    if (basic.mask)
    {
        size += 4;
    }
    return size;
}