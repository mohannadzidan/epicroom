#include "http/server.h"

class AppHttpServer : public HttpServer
{
public:
    AppHttpServer(bool tls);

protected:
    void handler(HttpRequest *request, HttpResponse *response) override;
    void receive(TCPConnection* connection) override;
};