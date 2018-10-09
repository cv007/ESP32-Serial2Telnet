#pragma once
#include <WiFi.h>

struct TelnetServer {
    typedef void (*handler)(WiFiClient&, bool);
    //bool = init? (can use if init needed when first connected)
    //when connected, handler is called with bool=true
    //normal handler call is bool=false

    TelnetServer(int port, const char* nam, handler h);
    void start();
    void stop_client();
    void check();

    private:

    void check_client();
    void check_data();

    WiFiServer      m_server;
    WiFiClient      m_client;
    int             m_port;
    const char*     m_name;
    bool            m_client_connected;
    IPAddress       m_client_ip;
    handler         m_handler;
};
