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

unsigned int privkey1_pem_len = 301;

WOLFSSL_CTX *ctx;
TCPServer *server;
void wolfssl_init()
{
    // Initialize WolfSSL library
    // wolfSSL_Debugging_ON();
    wolfSSL_Init();

    // Create a new SSL context
    ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method());
    if (!ctx)
    {
        printf("Failed to create SSL context\n");
        return;
    }

    File file;

    if (!readFile("/certs/privkey.pem", &file))
    {
        log_e("privkey.pem not found!");
        return;
    }
    int loadKeyResult = wolfSSL_CTX_use_PrivateKey_buffer(ctx, file.content, file.size, WOLFSSL_FILETYPE_PEM);
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
    int loadCertResult = wolfSSL_CTX_use_certificate_chain_buffer(ctx, file.content, file.size);
    // Load the server certificate
    if (loadCertResult != SSL_SUCCESS)
    {
        printf("Failed to load certificate code=%d\n", loadCertResult);
        return;
    }

    // if (wolfSSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) != SSL_SUCCESS) {
    //     printf("Failed to load private key\n");
    //     return;
    // }
}

TCPServer::TCPServer()
{

    connections = nullptr;
}

TCPServer::~TCPServer()
{
    TCPConnection *current = connections;
    while (current)
    {
        TCPConnection *next = current->linked_next;
        delete current;
        current = next;
    }
    free(connections);
    // if (state)
    // {
    //     free(state);
    // }
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
    server = this;
    wolfssl_init();
    tcp_arg(pcb, this);
    tcp_accept(pcb, &TCPServer::accept);
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
    printf("--- data sent ----\n");
    return ERR_OK;
}

err_t TCPServer::accept(void *serverPointer, tcp_pcb *client_pcb, err_t err)
{
    ip4_addr_t ip;
    if (err != ERR_OK || client_pcb == NULL)
    {
        log_e("Failure in accept (client_pcb is null?)=%d error=%d close=%d", client_pcb == NULL, err, client_pcb == NULL && tcp_close(client_pcb));
        return ERR_VAL;
    }
    server = (TCPServer *)serverPointer;
    TCPConnection *conn = server->allocConnection();
    if (!conn)
    {
        log_e("Failed to allocate connection");
        tcp_close(client_pcb);
        return ERR_MEM;
    }
    int ret = 0;
    conn->ssl = wolfSSL_new(ctx);
    if (!conn->ssl)
    {
        log_e("Failed to allocate wolfSSL_new");
        conn->close();
        return ERR_MEM;
    }

    conn->client_pcb = client_pcb;
    conn->server = server;
    log_t("<-TCP-- New TCP connection %s:%d", ip4addr_ntoa(&client_pcb->remote_ip), client_pcb->remote_port);
    ret = wolfSSL_SetIO_LwIP(conn->ssl, client_pcb, TCPServer::recv, nullptr, conn);
    if (ret != 0)
    {
        log_e("Failed to allocate wolfSSL_SetIO_LwIP %d", err);
        conn->close();
        return ERR_MEM;
    }
    tcp_err(client_pcb, server->onError);
    // tcp_arg(client_pcb, conn);
    // tcp_recv(client_pcb, &TCPServer::recv);
    // tcp_err(client_pcb, NULL);
    return ERR_OK;
}

void TCPServer::onError(void *arg, err_t err)
{
    if (err == ERR_RST)
    {
        for (int i = 0; i < MAX_CONNECTIONS; i++)
        {
            TCPConnection *connection = &server->connections[i];
            if (connection->client_pcb != nullptr && ((void *)&connection->ssl->lwipCtx == arg))
            {
                log_t("clossing due to RST connection=%p", connection);
                connection->close();
            }
        }
    }
    else
    {
        log_e("LwIp error code=%d", err);
    }
}

err_t TCPServer::recv(void *connectionPtr, tcp_pcb *tpcb, pbuf *data, err_t err)
{
    if (err != ERR_OK || tpcb == NULL)
    {
        log_e("Failure in recv error=%d", err);
        return ERR_VAL;
    }
    TCPConnection *connection = (TCPConnection *)connectionPtr;
    int ret = 0;
    if (!connection->ssl->options.handShakeDone)
    {
        log_d("<-TCP-- incoming handshake acceptState=%d", connection->ssl->options.acceptState);
        wolfSSL_accept(connection->ssl);
        return ERR_OK;
    }

    if (!data)
    {

        log_d("Client closed the connection");
        connection->close();
        return ERR_OK;
    }

    cyw43_arch_lwip_check();
    int readLength = connection->read();
    if (readLength < 0)
    {
        connection->close();
        return ERR_CLSD;
    }
    if (readLength == 0)
    {
        log_d("connection closed");
        connection->close();
        return ERR_OK;
    }
    connection->recv_len = readLength;
    connection->buffer_recv[readLength] = '\0';
    // tcp_recved(tpcb, data->tot_len);
    // pbuf_free(data);
    HttpRequest request;
    HttpResponse response(connection);
    // printf("\n-------Data %d bytes--------\n", readLength);
    // for (int i = 0; i < connection->recv_len; i++)
    // {
    //     if (connection->buffer_recv[i] <= 0)
    //         printf("·");
    //     else
    //         printf("%c", connection->buffer_recv[i]);
    // }
    // printf("\n---------------------------\n\n");

    if (connection->ws)
    {
        connection->ws->handle();
        return ERR_OK;
    }
    else if (parseHttp(connection->buffer_recv, connection->recv_len, &request))
    {
        log_t("<-HTTP-- %s \"%s\" src=%s:%d handshake=%d connection=%p",
              request.method, request.path,
              ip4addr_ntoa(&tpcb->remote_ip), tpcb->remote_port, connection->ssl->options.handShakeDone, connection);
        if (!strcmp("/ws", request.path))
        {
            if (!WebSocket::handle_handshake(connection, &request))
            {
                log_e("ws handshake error");
            }
            return ERR_OK;
        }
        File f;
        if (!strcmp("GET", request.method) && (readFile("/web", request.path, &f) || readFile("/web", "/index.html", &f)))
        {
            const char *cacheControl = !strcmp(f.mime_type, "text/html") ? "no-cache, must-revalidate" : "public, max-age=31536000, immutable";
            response.status(200);
            response.header("Content-Encoding", "gzip");
            response.header("Connection", "keep-alive");
            response.header("Keep-Alive ", " timeout=30, max=200");
            response.header("Cache-Control", cacheControl);
            response.header("Content-Length", f.size);
            response.header("Content-Type", f.mime_type);
            response.send(f.content, f.size);
            return ERR_OK;
        }
    }

    response.status(418);
    response.header("Cache-Control", "no-cache, must-revalidate");
    response.header("Content-Length", 13);
    response.send((const uint8_t *)"I'm a teapot", 13, 0);
    return ERR_OK;
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