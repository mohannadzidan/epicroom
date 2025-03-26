#pragma once

#include <stdint.h>
#include <stddef.h>
#include "TCPServer.h"
#include "errors.h"

#define HTTP_VERSION "HTTP/1.1"
#define HTTP_CACHE_CONTROL_MASK 0x1
#define HTTP_CACHE_CONTROL_NO_CACHE 0x0
#define HTTP_CACHE_CONTROL_IMMUTABLE 0x1
#define HTTP_NO_CACHE "200 Ok"
#define HTTP_RESPONSE_HEADER_BUFFER 256

class TCPConnection;

class HttpRequest
{
public:
    const char *method;    // HTTP method (e.g., GET, POST)
    const char *path;      // Request path (e.g., /index.html)
    const char *version;   // HTTP version (e.g., HTTP/1.1)
    const char *headers;   // Pointer to the start of headers (optional)
    size_t headers_length; // Length of headers (optional)

    const char *body;
    size_t body_length;
    app_err header(const char *name, char *outputValue, size_t bufferSize);
};

class HttpResponse
{
public:
    HttpResponse(TCPConnection *connection);
    void status(u16_t statusCode);
    void header(const char *name, const char *value);
    void header(const char *name, u32_t value);
    void write(const uint8_t *data, u32_t length, u8_t flags = TCP_WRITE_FLAG_MORE | TCP_WRITE_FLAG_COPY);
    void send(const uint8_t *data, u32_t length, u8_t flags = TCP_WRITE_FLAG_COPY);
    void send();
    TCPConnection *connection;

private:
    uint8_t isStatusSent : 1;
    uint8_t isBodySent : 1;
    uint8_t isHeadersSent : 1;
};

bool parseHttp(uint8_t *payload, size_t length, HttpRequest *request);
