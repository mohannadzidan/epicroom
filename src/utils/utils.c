#include "utils/utils.h"
#include <string.h>
#include <stdint.h>
#include "utils/sha1.h"

// SHA-1 implementation (simplified for embedded systems)
void mbedtls_sha1(const unsigned char *input, size_t ilen, unsigned char output[20])
{
    SHA1_CTX ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, input, ilen);
    sha1_final(output, &ctx);
}

// Base64 encoding table
static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Base64 encode function
size_t base64_encode(const uint8_t *src, size_t len, char *dst, size_t dst_len)
{
    size_t i, j;
    uint8_t a, b, c;

    if (dst_len < ((len + 2) / 3) * 4)
    {
        return 0; // Output buffer too small
    }

    for (i = 0, j = 0; i < len; i += 3, j += 4)
    {
        a = src[i];
        b = (i + 1 < len) ? src[i + 1] : 0;
        c = (i + 2 < len) ? src[i + 2] : 0;

        dst[j] = base64_table[a >> 2];
        dst[j + 1] = base64_table[((a & 0x03) << 4) | (b >> 4)];
        dst[j + 2] = (i + 1 < len) ? base64_table[((b & 0x0F) << 2) | (c >> 6)] : '=';
        dst[j + 3] = (i + 2 < len) ? base64_table[c & 0x3F] : '=';
    }

    dst[j] = '\0'; // Null-terminate the string
    return j;
}