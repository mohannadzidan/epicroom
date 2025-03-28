#pragma once

#include "lwip/tcp.h"
#include "wolfssl/ssl.h"
#include "tcp/core.h"
#include "utils/linked.h"
#include "lwip/stats.h"

#define lwip_available_send_mem (lwip_stats.mem.used > TCP_SND_BUF ? 0 : TCP_SND_BUF - lwip_stats.mem.used)
#define MAX_CONNECTIONS MEMP_NUM_TCP_PCB

struct TCPConnection;

class DifferedWrite : public LinkedBase
{
public:
    TCPConnection *connection;
    const uint8_t *buffer;
    uint32_t size;
};

class TCPServer : public LinkedBase
{
public:
    TCPServer(bool tls);
    ~TCPServer();

    bool open(u16_t port);
    void run();
    TCPConnection *allocConnection();
    TCPConnection *connections;
    uint8_t buffer_recv[2048];
    int recv_len;
    int activeConnections;
    static void poll();
    static void writeAsync(TCPConnection *connection, const void* data, size_t len);
    virtual void receive(TCPConnection *connection) = 0;

private:
    tcp_pcb *pcb;
    bool tls;
    static LinkedList<DifferedWrite *> differedWrites;
    static err_t accept(void *arg, tcp_pcb *client_pcb, err_t err);
    static err_t recv(void *arg, tcp_pcb *tpcb, pbuf *p, err_t err);
    static void onError(void *arg, err_t err);
    static LinkedList<TCPServer *> allServers;
};
