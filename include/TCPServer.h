#ifndef TCP_H
#define TCP_H
#include "lwip/tcp.h"
#include "wolfssl/ssl.h"
#include "TCPConnection.h"

#define MAX_CONNECTIONS MEMP_NUM_TCP_PCB

struct TCPConnection;

class TCPServer
{
public:
    TCPServer();
    ~TCPServer();

    bool open(u16_t port);
    void run();
    TCPConnection *allocConnection();
    TCPConnection *getConnection(tcp_pcb *tpcb);
    TCPConnection *connections;
    int activeConnections;

private:
    tcp_pcb *pcb;
    static err_t accept(void *arg, tcp_pcb *client_pcb, err_t err);
    static err_t recv(void *arg, tcp_pcb *tpcb, pbuf *p, err_t err);
    static void onError(void *arg, err_t err);
};

#endif // TCP_H