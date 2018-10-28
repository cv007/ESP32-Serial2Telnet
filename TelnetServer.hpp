#pragma once
#include <WiFi.h>

struct TelnetServer {

    //currently only using SERIAL2 and INFO
    using serve_t = enum { SERIAL0, SERIAL1, SERIAL2, INFO };

    //port, name, type
    TelnetServer        (int, const char*, serve_t);

    void start          ();
    void stop           ();
    void check          ();
    void status         (WiFiClient&);
    void stop_client    ();

    private:

    using msg_t = enum : uint8_t { START, CHECK, STOP };
    void handler        (msg_t);
    void handler_info   (msg_t);
    void handler_uart   (msg_t);

    WiFiServer          m_server;
    WiFiClient          m_client;
    int                 m_port;
    const char*         m_name;
    bool                m_client_connected;
    IPAddress           m_client_ip;
    serve_t             m_serve_type;
    HardwareSerial&     m_serial;

    //TODO: add code to be able to change these settings (via info port)
    uint32_t            m_baud{230400};
    uint32_t            m_config{SERIAL_8N1};
    int8_t              m_rxpin{-1};
    int8_t              m_txpin{-1};
    bool                m_txrx_invert{false};

};

//TODO
/*
    be able to specify baud on initial setup, and also later
    currently baud is fixed to 230400

    will have to do a m_serial.end()/m_serial.begin(baud) to change baud
    can just end/begin anywhere as Serial/1/2 are global
    Commander-
        serial 0 baud 115200
        serial 2 baud 19200
*/
