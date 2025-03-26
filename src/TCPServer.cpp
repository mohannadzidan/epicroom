#include <stdio.h>
#include "TCPServer.h"
#include "WebSocket.h"
#include "pico/cyw43_arch.h"
#include "log.h"
#include "fs.h"
#include "http.h"
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include "wolfssl/internal.h"
#include "errors.h"
#include "hardware/watchdog.h"

// #define LOG_CONNECTION_POLL

LinkedList<TCPServer *> TCPServer::allServers = LinkedList<TCPServer *>();

WOLFSSL_CTX *tlsCtx;
void loadSSLCertificates()
{
    File file;

    // Initialize WolfSSL library
    // wolfSSL_Debugging_ON();
    wolfSSL_Init();

    // Create a new SSL context
    tlsCtx = wolfSSL_CTX_new(wolfTLSv1_2_server_method());
    if (!tlsCtx)
    {
        printf("Failed to create SSL context\n");
        return;
    }
    watchdog_update();
    if (!readFile("/certs/privkey.pem", &file))
    {
        log_e("privkey.pem not found!");
        return;
    }
    int loadKeyResult = wolfSSL_CTX_use_PrivateKey_buffer(tlsCtx, file.content, file.size, WOLFSSL_FILETYPE_PEM);
    watchdog_update();
    // Load the private key
    if (loadKeyResult != SSL_SUCCESS)
    {
        printf("Failed to load private key code=%d\n", loadKeyResult);
        return;
    }

    if (!readFile("/certs/fullchain.pem", &file))
    {
        log_e("fullchain.pem not found!");
        return;
    }
    int loadCertResult = wolfSSL_CTX_use_certificate_chain_buffer(tlsCtx, file.content, file.size);
    watchdog_update();

    // Load the server certificate
    if (loadCertResult != SSL_SUCCESS)
    {
        printf("Failed to load certificate code=%d\n", loadCertResult);
        return;
    }
}

TCPServer::TCPServer(bool tls) : tls(tls), connections(nullptr), recv_len(0)
{
    allServers.add(this);
}

TCPServer::~TCPServer()
{
    log_t("free TCP server %p", this);
    TCPConnection *current = connections;
    while (current)
    {
        TCPConnection *next = current->linked_next;
        delete current;
        current = next;
    }
    allServers.remove(this);
}

void TCPServer::poll()
{

    uint32_t msSinceBoot = to_ms_since_boot(get_absolute_time());
    for (auto tcpServer : allServers)
    {
        TCPConnection *connection = tcpServer->connections;
        while (connection)
        {
            TCPConnection *next = connection->linked_next;
            if (connection->timeout != 0 && connection->active)
            {
                connection->expiresAt = msSinceBoot + connection->timeout;
                connection->active = false;
            }
#ifdef LOG_CONNECTION_POLL
            log_t("connection %p closeReason=%s timeout=%d ttl=%u",
                  connection,
                  connection->client_pcb == nullptr                                 ? "application"
                  : connection->timeout != 0 && msSinceBoot > connection->expiresAt ? "timeout"
                                                                                    : "none",
                  connection->timeout,
                  connection->expiresAt - msSinceBoot);
#endif
            if (!connection->client_pcb ||
                (connection->timeout != 0 && msSinceBoot > connection->expiresAt))
            {
                delete connection;
            }
            connection = next;
        }
    }
}

TCPConnection *TCPServer::allocConnection()
{
    return new TCPConnection(this);
}

bool TCPServer::open(u16_t port)
{
    log_i("Starting TCP server...");
    ip4_addr_t ip;
    ip = cyw43_state.netif[CYW43_ITF_STA].ip_addr;
    pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb)
    {
        log_e("Failed to create PCB");
        return false;
    }
    err_t err = tcp_bind(pcb, NULL, port);
    if (err)
    {
        log_e("Failed to bind to port %u", port);
        return false;
    }

    pcb = tcp_listen_with_backlog(pcb, 1);
    if (!pcb)
    {
        log_e("Failed to listen");
        tcp_close(pcb);
        return false;
    }
    if (tls && !tlsCtx)
    {
        loadSSLCertificates();
    }
    tcp_accept(pcb, &TCPServer::accept);
    tcp_arg(pcb, this);
    log_i("TCP server started on %s:%u", ip4addr_ntoa(&ip), port);
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

err_t sent(void *arg, tcp_pcb *tpcb, u16_t len)
{
    auto connection = reinterpret_cast<TCPConnection *>(arg);
    watchdog_update();
    connection->keepAlive();
    return ERR_OK;
}

err_t TCPServer::accept(void *serverPointer, tcp_pcb *client_pcb, err_t err)
{
    watchdog_update();
    ip4_addr_t ip;
    if (err != ERR_OK || client_pcb == NULL)
    {
        log_e("Failure in accept (client_pcb is null?)=%d error=%d close=%d", client_pcb == NULL, err, client_pcb == NULL && tcp_close(client_pcb));
        return ERR_VAL;
    }
    auto server = reinterpret_cast<TCPServer *>(serverPointer);
    TCPConnection *conn = server->allocConnection();
    if (!conn)
    {
        log_e("Failed to allocate connection");
        tcp_close(client_pcb);
        return ERR_MEM;
    }
    int ret = 0;

    conn->client_pcb = client_pcb;
    conn->server = server;
    if (server->tls)
    {
        conn->ssl = wolfSSL_new(tlsCtx);
        if (!conn->ssl)
        {
            log_e("Failed to allocate wolfSSL_new");
            conn->close();
            return ERR_MEM;
        }
        ret = wolfSSL_SetIO_LwIP(conn->ssl, client_pcb, recv, sent, conn);
        if (ret != 0)
        {
            log_e("Failed to allocate wolfSSL_SetIO_LwIP %d", err);
            conn->close();
            return ERR_MEM;
        }
    }
    else
    {
        conn->ssl = nullptr;
        tcp_recv(client_pcb, recv);
        tcp_sent(client_pcb, sent);
        tcp_arg(client_pcb, conn);
    }
    log_t("<-TCP-- New TCP connection %p %s:%d", conn, ip4addr_ntoa(&client_pcb->remote_ip), client_pcb->remote_port);
    tcp_err(client_pcb, server->onError);
    // tcp_arg(client_pcb, conn);
    // tcp_recv(client_pcb, &TCPServer::recv);
    // tcp_err(client_pcb, NULL);
    return ERR_OK;
}

void TCPServer::onError(void *arg, err_t err)
{
    watchdog_update();
    if (err == ERR_RST)
    {
        auto ctx = reinterpret_cast<WOLFSSL_LWIP_NATIVE_STATE *>(arg);
        auto connection = reinterpret_cast<TCPConnection *>(ctx->arg);
        log_t("connection %p has been reset", connection);
        connection->close();
    }
    else
    {
        log_e("LwIp error code=%d", err);
    }
}

err_t TCPServer::recv(void *connectionPtr, tcp_pcb *tpcb, pbuf *data, err_t err)
{
    watchdog_update();
    cyw43_arch_lwip_check();
    if (err != ERR_OK || tpcb == NULL)
    {
        log_e("Failure in recv error=%d", err);
        return ERR_VAL;
    }
    auto connection = reinterpret_cast<TCPConnection *>(connectionPtr);
    auto server = connection->server;
    int ret = 0;
    if (connection->ssl && !connection->ssl->options.handShakeDone)
    {
        wolfSSL_accept(connection->ssl);
        return ERR_OK;
    }

    if (!data || data->tot_len == 0)
    {

        log_d("Client closed the connection");
        connection->close();
        return ERR_OK;
    }
    if (connection->ssl)
    {

        int readLength = connection->read();
        if (readLength <= 0)
        {
            log_d("connection closed readLength=%d", readLength);
            connection->close();
            return ERR_OK;
        }
        server->recv_len = readLength;
        server->buffer_recv[readLength] = '\0';
    }
    else
    {
        // Receive the buffer
        const uint16_t buffer_left = BUF_SIZE - server->recv_len;
        server->recv_len += pbuf_copy_partial(data, server->buffer_recv, data->tot_len, 0);
        tcp_recved(tpcb, data->tot_len);
        pbuf_free(data);
    }
    server->receive(connection);
    return ERR_OK;
}
