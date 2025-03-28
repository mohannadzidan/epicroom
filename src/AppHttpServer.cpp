#include "AppHttpServer.h"
#include "utils/log.h"
#include "utils/fs.h"

AppHttpServer::AppHttpServer(bool tls) : HttpServer(tls) {}

void AppHttpServer::receive(TCPConnection *connection)
{
    if (connection->ws)
    {
        connection->ws->handle();
        return;
    }
    else
    {
        HttpServer::receive(connection);
    }
}

void AppHttpServer::handler(HttpRequest *request, HttpResponse *response)
{
    File file;
    if (!strcmp("/ws", request->path))
    {
        if (!WebSocket::handle_handshake(response->connection, request))
        {
            log_e("ws handshake error");
        }
        return;
    }
    if (!strcmp("GET", request->method) && (readFile("/web", request->path, &file) || readFile("/web", "/index.html", &file)))
    {
        const char *cacheControl = !strcmp(file.mime_type, "text/html") ? "no-cache, must-revalidate" : "public, max-age=31536000, immutable";
        response->connection->timeout = 30000;
        response->status(200);
        response->header("Content-Encoding", "gzip");
        response->header("Connection", "keep-alive");
        response->header("Keep-Alive", "timeout=30, max=200");
        response->header("Cache-Control", cacheControl);
        response->header("Content-Length", file.size);
        response->header("Content-Type", file.mime_type);
        if (lwip_available_send_mem >= file.size)
        {
            response->send(file.content, file.size);
        }
        else
        {
            log_t("send file %s differed", request->path);
            response->send();
            TCPServer::writeAsync(response->connection, reinterpret_cast<const void *>(file.content), file.size);
        }
    }
}
