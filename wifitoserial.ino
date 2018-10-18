/*
  UART to Telnet Server for ESP32

  lights-
    green - on = connected to wifi
            blinking slow = trying to connect to wifi
            blinkng fast = access point mode

  buttons-
    sw1 -   reset switch (case protruding)
    sw2 -   boot switch (case recessed)
            press and hold to go into access point mode
            press and hold, then press reset switch to go into bootloader mode

  connectoions-
    uart0 - recessed connection for wifi debug/programming
    uart1 - programming connector
    uart2 - programming connector


  initial setup- (initial firmware loaded)
        power on
            delay 5 seconds
            if boot2ap flag on OR no wifi credentials stored-
                start access point mode with telnet info port 2300 in use
                boot2ap flag is cleared

            try to connect to available wifi access points using stored credentials
                if unable, keep trying
                telnet port 2300 = info
                telnet port 2302 = uart2

            if sw2 pressed >3 sec, reboot to access point mode
                if unable to connect to any wifi (and credentials are available),
                user needs to press sw2 for > 3 seconds which will enable the boot2ap flag
                after releasing sw2, the esp will reboot

            if in access point mode-
                can connect to info on port 2300 to enter wifi credentials using console commands
                if sw2 pressed > 3 sec, reboot




  console commands-             //if completed, will have comments
                                //no comments = just an idea yet

  credentials list              //list all stored ssid/pass data
  credentials add 0 ssid=test1  //store ssid in slot 0
  credentials add 0 pass=pass1  //store password in slot 0
  credentials erase 0           //erase ssid/pass of slot 0
  boot2ap                       //show boot2ap flag status

  credentials set keyname=string

  net info
  net hostname                  //show hostname (dhcp server will record this name)
  net hostname=myname           //set hostname (max 32 chars)
  net APname                    //show APname (ssid name for access point mode)
  net APname=myAPname           //set APname (max 32 chars)
  net mac                       //show mac address


*/
#include <WiFi.h>
#include <WiFiMulti.h>

#include "TelnetServer.hpp"
#include "WifiCredentials.hpp"


WiFiMulti wifiMulti;


//boot0 switch- long press = run wifi access point
#include "Button.hpp"
Button sw1(0); //IO0


//handler for uart telnet connection
void telnet_uart_handler(WiFiClient& client, TelnetServer::msg_t msg)
{
    switch(msg){
        case TelnetServer::START:
            Serial2.begin(230400);
            Serial2.setTimeout(0); //set timeout for reads
            break;
        case TelnetServer::STOP:
            Serial2.end();
            break;;
        case TelnetServer::CHECK:
            size_t len;
            uint8_t buf[128];
            //check telnet client for data
            //get data from the telnet client and push it to the UART
            //Serial2.write is blocking, will complete-
            //max 5.5ms- 230400baud/128chars, max 11ms 115200baud/128chars
            len = client.available();
            if(len){
                if(len > 128) len = 128;
                client.read(buf, len);
                Serial2.write(buf, len);
            }

            //check UART for data, push it out to telnet
            len = Serial2.readBytes(buf, 128);
            if(len) client.write(buf, len);
    }

}

void boot2ap(WiFiClient& client)
{
    //"boot2ap"
    WifiCredentials wifidata;
    client.write("boot2ap: %d\n", wifidata.boot2ap());
}

void list_credentials(WiFiClient& client)
{
    uint8_t buf[256];
    WifiCredentials wifidata;
    client.printf("[##][ssid][pass]\n\n");
    for( uint8_t i = 0; i < wifidata.maxn(); i++ ){
        String s = wifidata.ssid(i);
        String p = wifidata.pass(i);
        client.printf("[%2d][%s][%s]\n", i, &s[0], &p[0]);
    }
}

void erase_credentials(WiFiClient& client, String s)
{
    WifiCredentials wifidata;
    //"credentials erase 0"
    s = s.substring(18);
    int idx = 0;
    if(s.substring(0,1) != "0"){
        idx = s.toInt();
        if(idx == 0){
            client.write("missing index or index not valid\n");
            return;
        }
    }
    if(idx > wifidata.maxn()){
        client.write("index is out of range\n");
        return;
    }
    wifidata.ssid(idx, "");
    wifidata.pass(idx, "");
}
void add_credentials(WiFiClient& client, String s)
{
    WifiCredentials wifidata;
    //"credentials add 0 ssid=myssid"
    //"credentials add 0 pass=mypass"
    s = s.substring(16);
    //0 ssid=myssid"
    //0 pass=mypass"
    int idx = 0;
    if(s.substring(0,2) != "0 "){
        idx = s.toInt();
        if(idx == 0){
            client.write("missing index or index not valid\n");
            return;
        }
    }
    if(idx > wifidata.maxn()){
        client.write("index is out of range\n");
        return;
    }

    int si = s.indexOf("ssid=");
    if(si > 0){
        wifidata.ssid(idx, &s[si+5]);
        return;
    }

    si = s.indexOf("pass=");
    if(si > 0){
        wifidata.pass(idx, &s[si+5]);
        return;
    }
    client.write("improper use of command\n");
}

void do_hostname(WiFiClient& client, String s)
{
    WifiCredentials wifidata;
    //"net hostname"
    s =s.substring(12);
    if(not s[0]){
        client.printf("%s\n", WiFi.getHostname());
        return;
    }
    //"net hostname=myname"
    if(s[0] == '='){
        s = s.substring(1);
        if(s.length() > 32){
            client.write("hostname too long (32 chars max)\n");
            return;
        }
        wifidata.hostname(s); //nvs storage
        WiFi.setHostname(s.c_str()); //and out to tcpip (may not see until reboot)
        return;
    }
    client.write("improper use of command\n");
}

void do_APname(WiFiClient& client, String s)
{
    WifiCredentials wifidata;
    //"net APname"
    s =s.substring(10);
    if(not s[0]){
        client.printf("%s\n", wifidata.APname().c_str());
        return;
    }
    //"net APname=myAPname"
    if(s[0] == '='){
        s = s.substring(1);
        if(s.length() > 32){
            client.write("APname too long (32 chars max)\n");
            return;
        }
        wifidata.APname(s); //nvs storage
        return;
    }
    client.write("improper use of command\n");
}

void get_mac(WiFiClient& client)
{
    client.printf("%s\n", WiFi.macAddress().c_str());
}

bool check_cmd(WiFiClient& client, uint8_t* buf, size_t len)
{
    uint8_t* p = buf; //buf will have 0 terminator
    while(*p >= ' ') p++; //find any char < space
    if(*p == 0) return false; //found 0 terminator, return and keep adding to buffer
    if(*p != '\r' && *p != '\n') return true; //no cr or lf, not a command
    //found a cr or lf, replace with 0 to terminate (exclude cr/lf)
    *p = 0;
    String s((const char*)buf); //to string
    //process s
    if(s.substring(0,16) == "credentials list"){ list_credentials(client); }
    else if(s.substring(0,16) == "credentials add "){ add_credentials(client, s); }
    else if(s.substring(0,7) == "boot2ap"){ boot2ap(client); }
    else if(s.substring(0,18) == "credentials erase "){ erase_credentials(client, s); }
    else if(s.substring(0,12) == "net hostname"){ do_hostname(client, s); }
    else if(s.substring(0,10) == "net APname"){ do_APname(client, s); }
    else if(s.substring(0,7) == "net mac"){ get_mac(client); }
    else { client.printf("invalid command\n"); }
    return true;
}

//handler for info telnet connection
void telnet_info_handler(WiFiClient& client, TelnetServer::msg_t msg)
{
    static uint8_t buf[128];
    static uint8_t idx;
    switch(msg){
        case TelnetServer::START:
            client.printf("Connected to info port\n\n$ ");
            break;
        case TelnetServer::STOP:
            break;
        case TelnetServer::CHECK:
            //check clients for data
            //get data from the telnet client

            //my telnet client seems to only send after a cr/lf
            //not sure what other telnet clients do

            size_t len = client.available();
            if(len){
                if(len+idx >= sizeof(buf)-1) len = sizeof(buf)-1-idx; //leave room for 0
                client.read(&buf[idx], len);
                //client.write(&buf[idx], len); //echo back
                idx += len; //increase index by length
                buf[idx] = 0; //0 terminate
                if(check_cmd(client, buf, sizeof(buf))){
                    idx = 0; //if \r or \n found, can reset index
                    client.write("$ ");
                }
                if(idx >= sizeof(buf)-1){
                    client.write("\n\ncommand too long :(\n\n$ "); //command buffer overflow
                    idx = 0; //start over
                }
            }

    }
}

//create telnet server for both
TelnetServer telnet_info(2300, "info", telnet_info_handler);
TelnetServer telnet_uart(2302, "uart2", telnet_uart_handler);

void ap_mode(){
    WifiCredentials wifidata;
    Serial.printf("\nstarting access point...");
    String APname = wifidata.APname();
    WiFi.softAP(APname[0] ? APname.c_str() : "SNAP-AP-telnet-2300");
    TelnetServer telnet_ap(2300, "apinfo", telnet_info_handler);
    Serial.printf("%s\n\n",WiFi.softAPIP().toString().c_str());
    delay(5000);
    telnet_ap.start();
    for(;;){
        telnet_ap.check();
        //check switch
        if(sw1.pressed() > 3000){
            Serial.printf("BOOT switch pressed, rebooting...\n");
            delay(2000);
            while(sw1.pressed() > 3000);
            telnet_ap.stop();
            ESP.restart();
        }
    }
}

//one time setup
void setup()
{
    Serial.begin(115200);
    Serial.printf("\nStartup delay... ");
    for(int i = 5; i > 0; Serial.printf("%d ", i), delay(1000), i--);
    Serial.printf("\n\n");

    WifiCredentials wifidata;
    if(wifidata.boot2ap()){
        //was flagged to boot to AP mode
        wifidata.boot2ap(false); //turn off if was on
        //start access point
        ap_mode();
    }

    bool found1 = false;
    for( uint8_t i = 0; i < wifidata.maxn(); i++ ){
        String s = wifidata.ssid(i);
        String p = wifidata.pass(i);
        if(not s.length()) continue; //if ssid blank, skip
        printf("using credentials from nvs storage...\n  %s  %s\n", &s[0], &p[0]);
        wifiMulti.addAP(&s[0], &p[0]);
        found1 = true;
    }
    if(not found1){
        Serial.printf("no credentials found in nvs storage\n");
        ap_mode();
    }
    Serial.printf("\n");

    //set hostname if stored
    WiFi.enableSTA(true); //needs to be on before setting hostname
    String hn = wifidata.hostname();
    if(hn[0]){
        WiFi.setHostname(hn.c_str());
        Serial.printf("setting hostname [%s] from nvs storage...\n\n", hn.c_str());
    } else {
        uint8_t mac[6];
        WiFi.macAddress(mac);
        char hn[11];
        snprintf(hn, 11, "SNAP-%02X%02X", mac[4],mac[5]);
        WiFi.setHostname(hn);
        Serial.printf("setting default hostname [%s]\n\n", hn);
    }

    Serial.printf("Connecting wifi...");
    for(int i = 10; i > 0; delay(1000), i--){
        if(wifiMulti.run() != WL_CONNECTED){
            Serial.printf("%d.", i);
            continue;
        }
        Serial.printf("connected [%s]\n", WiFi.localIP().toString().c_str());
        break;
    }

    Serial.printf("\n");
    if(wifiMulti.run() != WL_CONNECTED){
        Serial.printf("connect failed\n");
        delay(10000);
        ESP.restart(); //for now, just reboot and start over
    }

    //start the servers
    telnet_info.start();
    telnet_uart.start();
}

//main loop
void loop()
{
    //check if connection lost
    if(wifiMulti.run() != WL_CONNECTED){
        Serial.printf("wifi connection lost :(\n");
        delay(10000);
        ESP.restart(); //for now, just reboot and start over
    }

    //let each server check client connections/data
    telnet_info.check();
    telnet_uart.check();

    //check switch
    uint32_t p = sw1.pressed();
    if(p > 3000){
        Serial.printf("BOOT switch long press %d ms\n",p);
        telnet_info.stop();
        telnet_uart.stop();
        WifiCredentials wifidata;
        wifidata.boot2ap(true);
        //wait for release so sw is not pressed when rebooted
        //which would then boot into bootloader
        while(sw1.pressed() > 3000);
        ESP.restart();
    }
}
