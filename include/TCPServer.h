#ifndef TCP_H
#define TCP_H
#include "lwip/tcp.h"
#include "wolfssl/ssl.h"
#include "TCPConnection.h"
#include "linked.h"

#define MAX_CONNECTIONS MEMP_NUM_TCP_PCB

// #define iterate_server_connections(server, code)        \
//     {                                                   \
//         TCPConnection *current = server->connections;   \
//         while (current)                                 \
//         {                                               \
//             TCPConnection *next = current->linked_next; \
//             {code};                                     \
//             current = next;                             \
//         }                                               \
//     }



struct TCPConnection;

class TCPServer : public LinkedBase
{
public:
    TCPServer();
    ~TCPServer();

    bool open(u16_t port);
    void run();
    TCPConnection *allocConnection();
    TCPConnection *connections;
    uint8_t buffer_recv[2048];
    int recv_len;
    int activeConnections;
    static void poll();

private:
    tcp_pcb *pcb;
    static err_t accept(void *arg, tcp_pcb *client_pcb, err_t err);
    static err_t recv(void *arg, tcp_pcb *tpcb, pbuf *p, err_t err);
    static void onError(void *arg, err_t err);
    static LinkedList<TCPServer> allServers;
};

#endif // TCP_H