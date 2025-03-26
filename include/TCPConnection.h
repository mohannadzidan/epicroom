
#pragma once
#include <iterator>
#include "lwip/tcp.h"
#include "wolfssl/ssl.h"
#include "TCPServer.h"
#include "WebSocket.h"

#define BUF_SIZE (1024 * 2)
class TCPServer;
class WebSocket;
class TCPConnection
{

public:
    u8_t active : 1;
    u32_t timeout;
    u32_t expiresAt;
    TCPServer *server;
    tcp_pcb *client_pcb;
    WebSocket *ws;
    // uint8_t buffer_recv[BUF_SIZE];
    // int recv_len;
    err_t write(const void *data, size_t len);
    err_t write(const void *data, size_t len, u8_t flags);
    int read();
    err_t close();
    WOLFSSL *ssl;
    void free();
    TCPConnection(TCPServer *server);
    ~TCPConnection();
    static int activeCount;
    TCPConnection *linked_next;
    TCPConnection *linked_previous;
    void keepAlive();

    class Iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = TCPConnection *;
        using difference_type = std::ptrdiff_t;
        using pointer = TCPConnection **;
        using reference = TCPConnection *&;

        Iterator(TCPConnection *ptr = nullptr) : current(ptr) {}

        // Safe dereference - returns nullptr if current is nullptr
        TCPConnection *operator*() const { return current; }

        // Safe arrow operator - returns nullptr if current is nullptr
        TCPConnection *operator->() { return current; }

        // Prefix increment with nullptr check
        Iterator &operator++()
        {
            if (current)
            {
                current = current->linked_next;
            }
            return *this;
        }

        // Postfix increment with nullptr check
        Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const Iterator &a, const Iterator &b)
        {
            return a.current == b.current;
        }

        friend bool operator!=(const Iterator &a, const Iterator &b)
        {
            return a.current != b.current;
        }

    private:
        TCPConnection *current;
    };

    Iterator begin() { return Iterator(this); }
    Iterator end() { return Iterator(nullptr); }

private:
    uint8_t wantReadWriteTimes;
};
