#include "TelnetServer.hpp"
#include "Commander.hpp"
#include "NvsSettings.hpp"

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
             Serial) //INFO will set as Serial, unused
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
    //check for new clients, dropped clients
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

    //check handler if have client
    if(m_client) handler(CHECK);
    //else no client, so stop if not already done
    else if(m_client_connected) stop_client();
}

//if handler called with START/CHECK, m_client must be true
//if called with STOP, m_client is false, so no using m_client in STOP
void TelnetServer::handler(msg_t msg)
{
    if(m_serve_type == INFO) handler_info(msg);
    else handler_uart(msg);
}

void TelnetServer::handler_info(msg_t msg)
{
    switch(msg){
        case START:
            m_client.printf("\nConnected to info port %d (type ? for help)\n\n$ ", m_port);
            break;
        case STOP:
            break;
        case CHECK:
            static String s;
            static const auto maxlen = 127;
            size_t len = m_client.available();
            if(len){
                char c = 0;
                //read up to len bytes, while less than max cmd size
                for(; len && s.length() < maxlen; len--){
                    c = m_client.read(); //get 1 byte
                    if(c >= ' '){ s += c; continue; } //if a char, add and keep going
                    if(c == '\r' || c == '\n') break; //found command end
                    //special char, not cr/lf
                    //clear string
                    s = "";
                }
                //check last char- if cr or lf, process
                if(c == '\r' || c == '\n'){
                    s.trim();
                    if(s.length()){
                        Commander::process(m_client, s);
                        s = "";
                        m_client.printf("\n$ ");
                    }
                }
                //if too many chars
                if(s.length() >= maxlen){
                    m_client.printf("\n\ncommand too long :(\n\n$ "); //command buffer overflow
                    s = "";
                }
            }
            break;
    }
}

void TelnetServer::handler_uart(msg_t msg)
{
    switch(msg){
        case START:
//Serial.printf("%s START\n",__FUNCTION__);
            m_serial.begin(m_baud, m_config, m_rxpin, m_txpin, m_txrx_invert);
            //set serial timeout for reads
            m_serial.setTimeout(0);
            break;
        case TelnetServer::STOP:
//Serial.printf("%s STOP\n",__FUNCTION__);
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
//Serial.printf("%s CHECK telnet %d bytes\n",__FUNCTION__, len);
                if(len > 128) len = 128;
                m_client.read(buf, len);
                m_serial.write(buf, len);
            }
            //check UART for data, push it out to telnet
            len = m_serial.readBytes(buf, 128);
            if(len){
//Serial.printf("%s CHECK uart2 %d bytes\n",__FUNCTION__, len);
                m_client.write(buf, len);
            }
            break;
    }
}

