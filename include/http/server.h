#pragma once

#include "tcp/core.h"
#include "tcp/server.h"

class HttpServer : public TCPServer
{
public:
    HttpServer(bool tls);
    ~HttpServer();

protected:
    virtual void handler(HttpRequest* request, HttpResponse* response) = 0;
    virtual void receive(TCPConnection *connection) override;
};
