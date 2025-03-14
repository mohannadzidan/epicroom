#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Compute SHA-1 hash of input data
void mbedtls_sha1(const unsigned char *input, size_t ilen, unsigned char output[20]);

// Base64 encode data
size_t base64_encode(const uint8_t *src, size_t len, char *dst, size_t dst_len);

#ifdef __cplusplus
}
#endif

#endif // UTILS_H