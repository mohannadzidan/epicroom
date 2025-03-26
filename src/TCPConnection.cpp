
#include "TCPConnection.h"
#include "errors.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"
#include "log.h"
#include "hardware/watchdog.h"

int TCPConnection::activeCount = 0;

TCPConnection::TCPConnection(TCPServer *server)
{
    this->server = server;
    // this->recv_len = 0;
    this->ssl = nullptr;
    this->client_pcb = nullptr;
    this->ws = nullptr;
    this->timeout = 30000;
    this->expiresAt = UINT32_MAX;
    activeCount++;
    if (server->connections == nullptr)
    {
        // log_t("add connection %p to head", this);
        server->connections = this;
        this->linked_next = nullptr;
        this->linked_previous = nullptr;
    }
    else
    {
        // Link the new node to the current head
        // log_t("add connection %p to head and before %p", this, server->connections);
        this->linked_next = server->connections;
        this->linked_previous = nullptr;
        // Link the current head back to the new node
        server->connections->linked_previous = this;
        // Update head to point to the new node
        server->connections = this;
    }
}

TCPConnection::~TCPConnection()
{

    close();
    free();
    // If node to remove is the head node
    if (this == server->connections)
    {
        server->connections = this->linked_next;
        if (server->connections != nullptr)
        {
            // log_t("remove connection %p from head and new head is %p", this, server->connections);
            server->connections->linked_previous = nullptr;
        }
        else
        {
            // log_t("remove connection %p from head", this);
        }
    }
    else
    {
        // Update next node's previous pointer if it exists
        if (this->linked_next != nullptr)
        {
            // log_t("remove connection %p and update next.previous to be %p", this, this->linked_next);
            this->linked_next->linked_previous = this->linked_previous;
        }
        // Update previous node's next pointer if it exists
        if (this->linked_previous != nullptr)
        {
            // log_t("remove connection %p and update previous.next to be %p", this, this->linked_next);
            this->linked_previous->linked_next = this->linked_next;
        }
    }
    // Clear the removed node's pointers (optional)
    this->linked_next = nullptr;
    this->linked_previous = nullptr;
    activeCount--;
}

err_t TCPConnection::write(const void *buffer_send, size_t send_len, u8_t flags)
{
    watchdog_update();
    cyw43_arch_lwip_check();
    int ret = wolfSSL_write(this->ssl, buffer_send, send_len);
    keepAlive();
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
            log_t("connection closed %p due to WOLFSSL_ERROR_ZERO_RETURN", this);
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
    watchdog_update();
    cyw43_arch_lwip_check();
    int ret = wolfSSL_read(ssl, server->buffer_recv, sizeof(server->buffer_recv));
    keepAlive();
    while (ret == -1 && wantReadWriteTimes++ > 254)
    {
        ret = wolfSSL_get_error(ssl, ret);
        if (ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE)
        {
            ret = wolfSSL_read(ssl, server->buffer_recv, sizeof(server->buffer_recv));
        }
        else if (ret == WOLFSSL_ERROR_ZERO_RETURN)
        {
            // connection closed
            log_t("connection closed %p due to WOLFSSL_ERROR_ZERO_RETURN", this);
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
    watchdog_update();
    err_t err = ERR_OK;
    if (ws)
    {
        ws->writeCloseFrame();
    }
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
            log_e("Connection(%p) close failed err=%d, calling abort", this, err);
            tcp_abort(this->client_pcb);
            err = ERR_ABRT;
        }
        this->client_pcb = nullptr;
    }
    return err;
}

void TCPConnection::free()
{
    watchdog_update();
    if (ws)
    {
        log_t("Freeing TCPConnection(%p)->ws %p", this, ws);
        delete ws;
        ws = nullptr;
    }
    // recv_len = 0;
    if (ssl)
    {
        log_t("Freeing TCPConnection(%p)->ssl %p", this, ssl);
        wolfSSL_free(ssl);
        ssl = nullptr;
    }
}

inline void TCPConnection::keepAlive()
{
    active = 1;
}