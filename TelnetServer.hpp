#pragma once
#include <WiFi.h>

struct TelnetServer {
    typedef enum : uint8_t { START, CHECK, STOP } msg_t;
    typedef std::function<void(WiFiClient&, msg_t)> handler_func_t;

    TelnetServer(int port, const char* nam, handler_func_t h);
    void start();
    void stop();
    void check();

    private:

    void stop_client();
    void check_client();
    void check_data();

    WiFiServer      m_server;
    WiFiClient      m_client;
    int             m_port;
    const char*     m_name;
    bool            m_client_connected;
    IPAddress       m_client_ip;
    handler_func_t  m_handler;
};
