#pragma once

#include "lwip/tcp.h"
#include "wolfssl/ssl.h"
#include "TCPConnection.h"
#include "linked.h"
#include "TCPServer.h"

class HttpServer : public TCPServer
{
public:
    HttpServer(bool tls);
    ~HttpServer();

protected:
    virtual void handler(HttpRequest* request, HttpResponse* response) = 0;

private:
    void receive(TCPConnection *connection) override;
};
