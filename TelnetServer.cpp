#include "TelnetServer.hpp"
#include "Commander.hpp"

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

TelnetServer::TelnetServer(int port, const char* nam, serve_t typ)
    : m_server(port),
    m_port(port),
    m_name(nam),
    m_client_connected(false),
    m_serve_type(typ),
    m_serial(typ == TelnetServer::SERIAL0 ? Serial :
             typ == TelnetServer::SERIAL1 ? Serial1 :
             typ == TelnetServer::SERIAL2 ? Serial2 :
             Serial) //INFO will set as Serial, but will be unused
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
    if(not m_client_connected) return;          //was previously closed
    m_client_connected = false;                 //else update and print message
    info(m_name, "closed", m_port, m_client_ip);
    handler(STOP);                              //call handler
}

void TelnetServer::check()
{
    check_client();
    handler(CHECK);
}

void TelnetServer::check_client()
{
    if(m_server.hasClient()){
        if(m_client){                           //already have a client
            m_server.available().stop();        //so reject
            info(m_name, "rejected", m_port);
        } else {                                //can accept new client
            m_client = m_server.available();
            if (not m_client){                  //failed for some reason
                info(m_name, "failed", m_port);
                return;                         //failed to connect
            }
            m_client_connected = true;
            m_client_ip = m_client.remoteIP();
            info(m_name, "new client", m_port, m_client_ip);
            handler(START);                     //call handler
        }
    }
    if(not m_client && m_client_connected) stop_client(); //print message
}

void TelnetServer::handler(msg_t msg)
{
    if(not m_client) return;
    if(m_serve_type == INFO) handler_info(msg);
    else handler_uart(msg);
}

void TelnetServer::handler_info(msg_t msg)
{
    static uint8_t buf[128];
    static uint8_t idx;
    switch(msg){
        case START:
            m_client.printf("\nConnected to info port\n\n$ ");
            break;
        case STOP:
            break;
        case CHECK:
            //check clients for data
            //get data from the telnet client

            //my telnet client seems to only send after a cr/lf
            //not sure what other telnet clients do

            size_t len = m_client.available();
            if(len){
                if(len+idx >= sizeof(buf)-1) len = sizeof(buf)-1-idx; //leave room for 0
                m_client.read(&buf[idx], len);
                idx += len; //increase index by length
                buf[idx] = 0; //0 terminate
                if(Commander::process(m_client, buf)){
                    idx = 0; //if \r or \n found, can reset index
                }
                if(idx >= sizeof(buf)-1){
                    m_client.printf("\n\ncommand too long :(\n\n$ "); //command buffer overflow
                    idx = 0; //start over
                }
            }

    }

}
void TelnetServer::handler_uart(msg_t msg)
{
    switch(msg){
        case START:
            //set serial timeout for reads
            m_serial.begin(230400);
            m_serial.setTimeout(0);
            break;
        case TelnetServer::STOP:
            m_serial.end();
            break;
        case TelnetServer::CHECK:
            size_t len;
            uint8_t buf[128];
            //get data from the telnet client and push it to the UART
            //m_serial.write is blocking, will complete-
            //max 5.5ms- 230400baud/128chars, max 11ms 115200baud/128chars
            len = m_client.available();
            if(len){
                if(len > 128) len = 128;
                m_client.read(buf, len);
                m_serial.write(buf, len);
            }
            //check UART for data, push it out to telnet
            len = m_serial.readBytes(buf, 128);
            if(len) m_client.write(buf, len);
    }

}
