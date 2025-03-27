#include "HttpsRedirectionServer.h"
#include "utils/log.h"
#include "utils/fs.h"

HttpsRedirectionServer::HttpsRedirectionServer() : HttpServer(false) {}

void HttpsRedirectionServer::handler(HttpRequest *request, HttpResponse *response)
{
    char redirectUrl[256];
    if (request->header("Host", redirectUrl, sizeof(redirectUrl)) == APP_ERR_NONE)
    {
        // int i = strstr(http://)
        log_d("Host header found = '%s'", redirectUrl);
        char *end = strchr(redirectUrl, ':');
        if (end != nullptr)
        {
            *end = '\0';
        }
        memmove(redirectUrl + (sizeof("https://") - 1), redirectUrl, strlen(redirectUrl) + 1);
        memcpy(redirectUrl, "https://", (sizeof("https://") - 1));
        strcat(redirectUrl, request->path);
    }
    else
    {
        sprintf(redirectUrl, "https://%s%s",
                ipaddr_ntoa(&response->connection->client_pcb->local_ip),
                request->path);
    }
    response->status(301);
    response->header("Location", redirectUrl);
    response->header("Content-Length", "0");
    response->header("Connection", "close");
    response->connection->close();
}
