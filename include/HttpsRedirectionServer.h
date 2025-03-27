#include "http/server.h"

class HttpsRedirectionServer : public HttpServer
{
public:
    HttpsRedirectionServer();

protected:
    void handler(HttpRequest *request, HttpResponse *response) override;
};