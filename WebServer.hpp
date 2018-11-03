#pragma once

#include <WiFi.h>

struct WebServer : public WiFiServer {

    WebServer(uint16_t port, const char* name)
    : m_port(port), m_server(port, 1), m_name(name)
    {}

    void            start();
    void            check();
    void            stop();

    private:

    WiFiServer      m_server;
    uint16_t        m_port;
    const char*     m_name;
};
