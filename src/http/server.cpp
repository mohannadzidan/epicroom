#include "http/server.h"
#include "utils/log.h"
#include "http/core.h"
#include "hardware/watchdog.h"

HttpServer::HttpServer(bool tls) : TCPServer(tls)
{
}

HttpServer::~HttpServer()
{
}

void HttpServer::receive(TCPConnection *connection)
{
    watchdog_update();
    HttpRequest request;
    HttpResponse response(connection);
    if (parseHttp(connection->server->buffer_recv, connection->server->recv_len, &request))
    {

        log_t("<-HTTP-- %s \"%s\" src=%s:%d connection=%p",
              request.method,
              request.path,
              ip4addr_ntoa(&connection->client_pcb->remote_ip),
              connection->client_pcb->remote_port,
              connection);
        handler(&request, &response);
    }
    else
    {
        log_d("Received a request i don't understand data(string)='%.s'", connection->server->recv_len, connection->server->buffer_recv);
        printf("-------Data %d bytes--------\n", connection->server->recv_len);
        for (int i = 0; i < connection->server->recv_len; i++)
        {
            if (connection->server->buffer_recv[i] <= 0)
                printf("·");
            else
                printf("%c", connection->server->buffer_recv[i]);
        }
        printf("\n---------------------------\n\n");
        response.status(418);
        response.header("Cache-Control", "no-cache, must-revalidate");
        response.header("Content-Length", 13);
        response.send((const uint8_t *)"I'm a teapot", 13, 0);
    }
}