#include "TelnetServer.hpp"

//=====================
// local functions
//=====================

//print connection info
//
//Telnet Server |  uart | s192.168.123.100 | p   23 | c192.168.123.101 | starting
//
static void info(const char* nam, const char* msg, int port, IPAddress remip = {0,0,0,0})
{
    Serial.printf("Telnet Server | %5s | s%15s | p%5d | c%15s | %s\n",
        nam, WiFi.localIP().toString().c_str(), port, (uint32_t)remip?remip.toString().c_str():"", msg
    );
}

//=====================
// class functions
//=====================

TelnetServer::TelnetServer(int port, const char* nam, handler_func_t h)
    : m_server(port),
    m_port(port),
    m_name(nam),
    m_client_connected(false),
    m_handler(h)
{
}

void TelnetServer::start()
{
    info(m_name, "starting", m_port);
    m_server.begin();
    m_server.setNoDelay(true);
}

void TelnetServer::stop()
{
    info(m_name, "stopping", m_port);
    stop_client();
    m_server.end();
}

void TelnetServer::stop_client()
{
    if(m_client) m_client.stop();               //stop client if not already
    if(!m_client_connected) return;             //was previously closed
    m_client_connected = false;                 //else update and print message
    info(m_name, "closed", m_port, m_client_ip);
    m_handler(m_client, STOP);                  //call handler
}

void TelnetServer::check()
{
    check_client();
    check_data();
}

void TelnetServer::check_client()
{
    if(m_server.hasClient()){
        if(m_client){                           //already have a client
            m_server.available().stop();        //so reject
            info(m_name, "rejected", m_port);
        } else {                                //can accept new client
            m_client = m_server.available();
            if (!m_client){                     //failed for some reason
                info(m_name, "failed", m_port);
                return;                         //failed to connect
            }
            m_client_connected = true;
            m_client_ip = m_client.remoteIP();
            info(m_name, "new client", m_port, m_client_ip);
            m_handler(m_client, START);         //call handler
        }
    }
    if(!m_client && m_client_connected) stop_client(); //print message
}

void TelnetServer::check_data()
{
    if(m_client) m_handler(m_client, CHECK);   //call handler
}

