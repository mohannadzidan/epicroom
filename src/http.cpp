#include "http.h"
#include "TCPConnection.h"
#include "log.h"
#include <string.h>
#include "errors.h"

char headersBuffer[HTTP_RESPONSE_HEADER_BUFFER];

bool parseHttp(uint8_t *payload, size_t length, HttpRequest *request)
{
    // Ensure the payload is null-terminated for string operations
    if (payload == nullptr || length == 0)
    {
        return false;
    }

    // Cast payload to char* for easier string manipulation
    auto data = reinterpret_cast<char *>(payload);

    // Find the end of the first line (request line)
    char *line_end = strstr(data, "\r\n");
    if (!line_end)
    {
        return false; // Malformed request (no end of line)
    }

    // Null-terminate the request line for parsing
    *line_end = '\0';

    // Parse the request line (method, path, version)
    char *method = strtok(data, " ");
    char *path = strtok(nullptr, " ");
    char *version = strtok(nullptr, " ");

    if (!method || !path || !version)
    {
        return false; // Malformed request line
    }

    // Assign parsed values to the struct
    request->method = method;
    request->path = path;
    request->version = version;

    // Find the start of headers (right after the request line)
    char *headers_start = line_end + 2; // Skip "\r\n"
    char *headers_end = strstr(headers_start, "\r\n\r\n");

    if (!headers_end)
    {
        // No headers found, but the request is still valid
        request->headers = nullptr;
        request->headers_length = 0;
    }
    else
    {
        // Assign headers to the struct
        request->headers = headers_start;
        request->headers_length = headers_end - headers_start;

        // Find the start of the body (right after the headers)
        char *body_start = headers_end + 4; // Skip "\r\n\r\n"
        char *body_end = data + length;     // End of the payload

        // Assign body to the struct
        request->body = body_start;
        request->body_length = body_end - body_start;
    }

    return true;
}

void httpError(TCPConnection *connection, const char *error)
{
    int writtenHeadersLength = snprintf(headersBuffer, HTTP_RESPONSE_HEADER_BUFFER,
                                        "HTTP/1.1 %s\r\n"
                                        "Content-Type: text/plain\r\n"
                                        "Connection: close\r\n"
                                        "\r\n",
                                        error);
    connection->write(headersBuffer, writtenHeadersLength, TCP_WRITE_FLAG_COPY);
    connection->close();
}

HttpResponse::HttpResponse(TCPConnection *con)
{
    connection = con;
    isStatusSent = false;
    isHeadersSent = false;
    isBodySent = false;
    log_d("initi http response");
}

void HttpResponse::status(u16_t statusCode)
{
    if (isStatusSent)
    {
        log_e("status already sent");
        return;
    }
    int writtenHeadersLength = snprintf(headersBuffer, HTTP_RESPONSE_HEADER_BUFFER,
                                        HTTP_VERSION " %s\r\n",
                                        statusCode == 200   ? "200 Ok"
                                        : statusCode == 500 ? "500 Internal Server Error"
                                        : statusCode == 418 ? "418 I'm a teapot"
                                        : statusCode == 101 ? "101 Switching Protocols"
                                                            : "500 Internal Server Error");
    connection->write(headersBuffer, writtenHeadersLength, TCP_WRITE_FLAG_MORE);
    isStatusSent = true;
}

void HttpResponse::header(const char *name, const char *value)
{
    if (isBodySent)
    {
        log_e("body is already sent");
        return;
    }
    if (!isStatusSent)
    {
        status(200);
    }
    int writtenHeadersLength = snprintf(headersBuffer, HTTP_RESPONSE_HEADER_BUFFER, "%s: %s\r\n", name, value);
    connection->write(headersBuffer, writtenHeadersLength, TCP_WRITE_FLAG_MORE);
}

void HttpResponse::header(const char *name, uint32_t value)
{
    if (isBodySent)
    {
        log_e("body is already sent");
        return;
    }
    if (!isStatusSent)
    {
        status(200);
    }
    int writtenHeadersLength = snprintf(headersBuffer, HTTP_RESPONSE_HEADER_BUFFER, "%s: %d\r\n", name, value);
    connection->write(headersBuffer, writtenHeadersLength, TCP_WRITE_FLAG_MORE);
}

void HttpResponse::write(const uint8_t *data, u32_t length, u8_t flags)
{
    if (!isStatusSent)
    {
        status(200);
    }
    if (!isHeadersSent)
    {
        connection->write("\r\n", 2, TCP_WRITE_FLAG_MORE);
        isHeadersSent = true;
    }
    connection->write(data, length, flags | TCP_WRITE_FLAG_MORE);
}

void HttpResponse::send(const uint8_t *data, u32_t length, u8_t flags)
{
    if (!isStatusSent)
    {
        status(200);
    }
    if (isBodySent)
    {
        log_e("body already sent");
        return;
    }
    if (!isHeadersSent)
    {
        connection->write("\r\n", 2, TCP_WRITE_FLAG_MORE);
        isHeadersSent = true;
    }
    connection->write(data, length, flags & ~TCP_WRITE_FLAG_MORE);
    // connection->close();
    isBodySent = true;
}

void HttpResponse::send()
{
    if (!isStatusSent)
    {
        status(200);
    }
    if (isBodySent)
    {
        log_e("body already sent");
        return;
    }
    if (!isHeadersSent)
    {
        connection->write("\r\n", 2, TCP_WRITE_FLAG_MORE);
        isHeadersSent = true;
    }
    isBodySent = true;
}
/*
curl --include --no-buffer  https://epicroom.duckdns.org/ws \
     --header "Connection: Upgrade" \
     --header "Upgrade: websocket" \
     --header "Sec-WebSocket-Key: SGVsbG8sIHdvcmxkIQ==" \
     --header "Sec-WebSocket-Version: 13"

*/
app_err HttpRequest::header(const char *name, char *output, size_t bufferSize)
{
    const char *key_start = strstr(headers, name);
    if (!key_start)
    {
        return APP_ERR_NOT_FOUND;
    }
    key_start += strlen(name) + 2; // sizeof(": ")
    const char *end = strstr(key_start, "\r\n");
    if (!end)
    {
        return APP_ERR_NOT_FOUND;
    }
    size_t key_len = end - key_start;
    if (key_len > bufferSize)
    {
        return APP_ERR_TOO_LONG;
    }
    memcpy(output, key_start, key_len);
    output[key_len] = '\0';
    return APP_ERR_NONE;
}
