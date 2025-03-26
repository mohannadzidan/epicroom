#pragma once

#include "lwip/tcp.h"
#include "wolfssl/ssl.h"
#include "TCPConnection.h"
#include "linked.h"

#define MAX_CONNECTIONS MEMP_NUM_TCP_PCB

struct TCPConnection;

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

    virtual void receive(TCPConnection *connection) = 0;

private:
    tcp_pcb *pcb;
    static err_t accept(void *arg, tcp_pcb *client_pcb, err_t err);
    static err_t recv(void *arg, tcp_pcb *tpcb, pbuf *p, err_t err);
    static void onError(void *arg, err_t err);
    static LinkedList<TCPServer *> allServers;
    bool tls;
};
