#include "AppHttpServer.h"
#include "log.h"
#include "fs.h"

AppHttpServer::AppHttpServer(bool tls) : HttpServer(tls) {}

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
        response->header("Keep-Alive ", "timeout=30, max=200");
        response->header("Cache-Control", cacheControl);
        response->header("Content-Length", file.size);
        response->header("Content-Type", file.mime_type);
        response->send(file.content, file.size);
    }
}
