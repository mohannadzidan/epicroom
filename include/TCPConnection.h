
#pragma once
#include "lwip/tcp.h"
#include "wolfssl/ssl.h"
#include "TCPServer.h"
#include "WebSocket.h"

#define BUF_SIZE (1024 * 2)
class TCPServer;
class WebSocket;
class TCPConnection
{

public:
    TCPServer *server;
    tcp_pcb *client_pcb;
    WebSocket *ws;
    uint8_t buffer_recv[BUF_SIZE];
    int recv_len;
    err_t write(const void *data, size_t len);
    err_t write(const void *data, size_t len, u8_t flags);
    int read();
    err_t close();
    WOLFSSL *ssl;
    void free();
    TCPConnection(TCPServer *server);
    ~TCPConnection();
    static int activeCount;
    TCPConnection* linked_next;
    TCPConnection* linked_previous;
private:
    uint8_t wantReadWriteTimes;
};
