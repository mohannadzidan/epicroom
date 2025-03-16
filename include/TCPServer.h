#ifndef TCP_H
#define TCP_H
#include "lwip/tcp.h"

#define BUF_SIZE 2048

class TCPServer;

struct TCPConnection
{
    TCPServer *server;
    tcp_pcb *client_pcb;
    bool is_websocket;
    uint8_t buffer_recv[BUF_SIZE];
    int recv_len;
    err_t write(const void *data, size_t len);
    err_t write(const void *data, size_t len, u8_t flags);
    err_t close();
    void free();
};

class TCPServer
{
public:
    TCPServer();
    ~TCPServer();

    bool open(u16_t port);
    void run();
    void freeConnection(TCPConnection *conn);
    TCPConnection *allocConnection();
    TCPConnection *getConnection(tcp_pcb *tpcb);
    TCPConnection connections[MEMP_NUM_TCP_PCB];

private:
    tcp_pcb *pcb;
    static err_t accept(void *arg, tcp_pcb *client_pcb, err_t err);
    static err_t recv(void *arg, tcp_pcb *tpcb, pbuf *p, err_t err);
};

#endif // TCP_H