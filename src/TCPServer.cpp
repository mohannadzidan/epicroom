#include <stdio.h>
#include "TCPServer.h"
#include "WebSocket.h"

err_t TCPConnection::write(const void *buffer_send, size_t send_len, u8_t flags)
{
    cyw43_arch_lwip_check();
    err_t err = tcp_write(this->client_pcb, buffer_send, send_len, flags);
    if (err != ERR_OK)
    {
        printf("Failed to write data %d\n", err);
    }
    return err;
}

err_t TCPConnection::write(const void *buffer_send, size_t send_len)
{
    return TCPConnection::write(buffer_send, send_len, TCP_WRITE_FLAG_COPY);
}

err_t TCPConnection::close()
{
    err_t err = ERR_OK;
    if (this->client_pcb != NULL)
    {
        tcp_arg(this->client_pcb, NULL);
        tcp_poll(this->client_pcb, NULL, 0);
        tcp_sent(this->client_pcb, NULL);
        tcp_recv(this->client_pcb, NULL);
        tcp_err(this->client_pcb, NULL);
        err = tcp_close(this->client_pcb);
        if (err != ERR_OK)
        {
            printf("Close failed %d, calling abort\n", err);
            tcp_abort(this->client_pcb);
            err = ERR_ABRT;
        }
        this->free();
    }
    return err;
}

void TCPConnection::free()
{
    client_pcb = NULL;
    is_websocket = false;
    recv_len = 0;
}

TCPServer::TCPServer()
{
    for (int i = 0; i < sizeof(connections) / sizeof(TCPConnection); i++)
    {
        connections[i].free();
    }
    // state = (TCPConnection *)calloc(1, sizeof(TCPConnection));
    // if (!state)
    // {
    //     printf("Failed to allocate state\n");
    // }
}

TCPServer::~TCPServer()
{
    // if (state)
    // {
    //     free(state);
    // }
}

bool TCPServer::open(u16_t port)
{
    ip4_addr_t ip;
    ip = cyw43_state.netif[CYW43_ITF_STA].ip_addr;
    pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb)
    {
        printf("Failed to create PCB\n");
        return false;
    }

    err_t err = tcp_bind(pcb, NULL, port);
    if (err)
    {
        printf("Failed to bind to port %u\n", port);
        return false;
    }

    pcb = tcp_listen_with_backlog(pcb, 1);
    if (!pcb)
    {
        printf("Failed to listen\n");
        tcp_close(pcb);
        return false;
    }

    tcp_arg(pcb, this);
    tcp_accept(pcb, &TCPServer::accept);
    printf("TCP server started on %s:%u\n", ip4addr_ntoa(&ip), port);
    return true;
}

void TCPServer::run()
{
    while (true)
    {
#if PICO_CYW43_ARCH_POLL
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
#else
        sleep_ms(1000);
#endif
    }
}

err_t TCPServer::accept(void *serverPointer, tcp_pcb *client_pcb, err_t err)
{

    if (err != ERR_OK || client_pcb == NULL)
    {
        printf("Failure in accept\n");
        return ERR_VAL;
    }

    printf("Client connected\n");
    TCPServer *server = (TCPServer *)serverPointer;
    printf("the passed server  %p\n", server);
    TCPConnection *conn = server->allocConnection();
    if (!conn)
    {
        printf("Failed to allocate connection\n");
        return ERR_MEM;
    }

    conn->client_pcb = client_pcb;
    conn->server = server;

    tcp_arg(client_pcb, conn);
    tcp_recv(client_pcb, &TCPServer::recv);
    tcp_err(client_pcb, NULL);
    return ERR_OK;
}

err_t TCPServer::recv(void *connectionPtr, tcp_pcb *tpcb, pbuf *data, err_t err)
{
    TCPConnection *connection = (TCPConnection *)connectionPtr;

    if (!data)
    {
        printf("Client closed the connection\n");
        connection->close();
        return ERR_OK;
    }

    cyw43_arch_lwip_check();

    connection->recv_len = pbuf_copy_partial(data, connection->buffer_recv, data->tot_len, 0);
    connection->buffer_recv[connection->recv_len] = '\0';
    tcp_recved(tpcb, data->tot_len);
    pbuf_free(data);

    if (!connection->is_websocket)
    {
        if (!WebSocket::handle_handshake(connection, tpcb, (const char *)connection->buffer_recv, connection->recv_len))
        {
            printf("WebSocket handshake failed\n");
            char response[32];
            snprintf(response, sizeof(response), "HTTP/1.1 418 I'm a teapot\r\n");
            connection->write(response, sizeof(response));
            connection->close();
        }
    }
    else
    {
        WebSocket::handle_frame(connection, tpcb, connection->buffer_recv, connection->recv_len);
    }

    return ERR_OK;
}

void TCPServer::freeConnection(TCPConnection *conn)
{
    if (conn)
    {
        conn->client_pcb = NULL;
        conn->is_websocket = false;
    }
}

TCPConnection *TCPServer::allocConnection()
{
    for (int i = 0; i < MEMP_NUM_TCP_PCB; i++)
    {
        if (connections[i].client_pcb == NULL)
        {
            return &connections[i];
        }
    }
    return NULL;
}

TCPConnection *TCPServer::getConnection(tcp_pcb *tpcb)
{
    for (int i = 0; i < MEMP_NUM_TCP_PCB; i++)
    {
        if (connections[i].client_pcb == tpcb)
        {
            return &connections[i];
        }
    }
    return NULL;
}