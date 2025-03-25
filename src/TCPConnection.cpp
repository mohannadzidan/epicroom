
#include "TCPConnection.h"
#include "errors.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"
#include "log.h"

int TCPConnection::activeCount = 0;

TCPConnection::TCPConnection(TCPServer *server)
{
    this->server = server;
    this->recv_len = 0;
    this->ssl = nullptr;
    this->client_pcb = nullptr;
    this->ws = nullptr;
    this->linked_previous = nullptr;
    this->linked_next = nullptr;
    activeCount++;
    if (server->connections == nullptr)
    {
        server->connections = this;
    }
    else
    {
        server->connections->linked_next = this;
    }
}

TCPConnection::~TCPConnection()
{

    close();

    log_t("Freeing TCPConnection %p", this);
    client_pcb = NULL;
    if (ws)
    {
        log_t("Freeing ws %p", ws);
        delete ws;
        ws = NULL;
    }
    recv_len = 0;
    if (ssl)
    {
        log_t("Freeing ssl %p", ssl);
        wolfSSL_free(ssl);
    }

    ssl = NULL;
    log_t("Freed TCPConnection %p", this);

    if (this->linked_next && this->linked_previous)
    {
        // Root->P->O->N
        this->linked_previous->linked_next = this->linked_next;
    }
    else if (this->linked_previous)
    {
        // Root->P->O->null
        this->linked_previous->linked_next = nullptr;
    }
    else if (server->connections == this && this->linked_next)
    {
        // Root->O->N
        server->connections = this->linked_next ? this->linked_next : nullptr;
    }
    else if (server->connections == this)
    {
        // Root->O->null
        server->connections = this->linked_next ? this->linked_next : nullptr;
    }
    activeCount--;
}

err_t TCPConnection::write(const void *buffer_send, size_t send_len, u8_t flags)
{
    cyw43_arch_lwip_check();
    int ret = wolfSSL_write(this->ssl, buffer_send, send_len);
    while (ret == -1)
    {
        ret = wolfSSL_get_error(ssl, ret);
        if (ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE)
        {
            ret = wolfSSL_write(this->ssl, buffer_send, send_len);
        }
        else if (ret == WOLFSSL_ERROR_ZERO_RETURN)
        {
            // connection closed
            close();
            return APP_ERR_CONNECTION_CLOSED;
        }
        else
        {
            log_e("Fatal error write data err=%d", ret);
            return ret;
        }
    }
    return ERR_OK;
}

int TCPConnection::read()
{
    cyw43_arch_lwip_check();
    int ret = wolfSSL_read(ssl, buffer_recv, sizeof(buffer_recv));
    while (ret == -1 && wantReadWriteTimes++ > 254)
    {
        ret = wolfSSL_get_error(ssl, ret);
        if (ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE)
        {
            ret = wolfSSL_read(ssl, buffer_recv, sizeof(buffer_recv));
        }
        else if (ret == WOLFSSL_ERROR_ZERO_RETURN)
        {
            // connection closed
            close();
            return APP_ERR_CONNECTION_CLOSED;
        }
        else
        {
            log_e("Fatal error while read err=%d", ret);
            return APP_ERR_FATAL;
        }
    }
    return ret;
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
            log_e("Close failed %d, calling abort", err);
            tcp_abort(this->client_pcb);
            err = ERR_ABRT;
        }
        this->free();
    }
    return err;
}

void TCPConnection::free()
{
    server->activeConnections--;
    log_t("Freeing TCPConnection %p", this);
    client_pcb = NULL;
    if (ws)
    {
        log_t("Freeing ws %p", ws);
        delete ws;
        ws = NULL;
    }
    recv_len = 0;
    if (ssl)
    {
        log_t("Freeing ssl %p", ssl);
        wolfSSL_free(ssl);
    }

    ssl = NULL;
    log_t("Freed TCPConnection %p", this);
}