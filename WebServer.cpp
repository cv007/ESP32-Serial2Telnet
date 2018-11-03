#include "WebServer.hpp"
#include "Commander.hpp"

//=====================
// local functions
//=====================

//print connection info
//
//Web Server    |  uart | s192.168.123.100 | p   23 | c192.168.123.101 | starting
//
static void info(const char* nam, const char* msg, int port, IPAddress remip = {0,0,0,0})
{
    Serial.printf("Web Server    | %5s | s%15s | p%5d | c%15s | %s\n",
        nam, WiFi.localIP().toString().c_str(), port, (uint32_t)remip?remip.toString().c_str():"", msg
    );
}

//constant strings
const char* HTTP_OK = "HTTP/1.1 200 OK";
const char* HTTP_404 = "HTTP/1.1 404 ";
const char* HTTP_TXT = "Content-type:text/plain";


//=====================
// class functions
//=====================

void WebServer::start()
{
    info(m_name, "starting", m_port);
    m_server.begin();
    m_server.setNoDelay(true);
}

void WebServer::stop()
{
    info(m_name, "stopping", m_port);
    m_server.end();
}

void WebServer::check()
{
  WiFiClient client = m_server.available(); //incoming clients

  if (client) {                             //there is a client
    info(m_name, "new client", m_port, client.remoteIP());
    String currentLine = "";                //store each line
    String cmd = "help";                    //default command
    while (client.connected()) {
      if (client.available()) {             //if client bytes available
        char c = client.read();             //read byte
        if(c == '\r') continue;             //ignore CR
        if(c != '\n'){                      //save char unless LF
            currentLine += c;
            continue;                       //next
        }

        //is LF- DONE with line

        //Serial.printf(">> %s\n", currentLine.c_str());

        //first LF, check the line
        if(currentLine.length()){
            // Check to see if the client request was GET /'something here'
            if(currentLine.startsWith("GET /'")){
                currentLine.replace("GET /'", "");
                currentLine.replace("%20"," ");
                auto i = currentLine.indexOf("'");
                if(i>=0){
                    cmd = currentLine.substring(0, i);
                    cmd.trim();
                    Serial.printf("Web server command received: %s\n", cmd.c_str());
                }
            }
            else if (currentLine.startsWith("GET /favicon.ico")){
                cmd = "favicon";
            }
            currentLine = ""; //done checking line, clear string for next line
        }
        //second LF, end of HTTP request
        else {
            //this should stop all icon requests after the first time
            if(cmd == "favicon"){
                client.println(HTTP_404);
                client.println();
                break;
            }

            client.println(HTTP_OK);                    //header
            client.println(HTTP_TXT);                   //content type
            client.println();                           //separator

            //now let commander process the command (prints directly to client)
            Commander::process(client, cmd);            //default is "help"
            if(cmd == "help"){                          //add additional info for help
                client.println("\n\nappend command to address in single quotes-");
                client.println("http://192.168.4.1/'wifi list'");
                client.println("\n(or use telnet command interface via port 2300)");
            }
            client.println();
            break;
        }
      }
    }
    client.stop();
    info(m_name, "closed", m_port);
  }
}

