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




  console commands-

  credentials list              //list all stored ssid/pass data
  credentials add 0 ssid=test1  //store ssid in slot 0
  credentials add 0 pass=pass1  //store password in slot 0
  credentials erase 0           //erase ssid/pass of slot 0
  boot2ap                       //show boot2ap flag status





*/
#include <WiFi.h>
#include <WiFiMulti.h>

#include "TelnetServer.hpp"
#include "WifiCredentials.hpp"


WiFiMulti wifiMulti;


//boot0 switch- long press = run wifi access point
#include "Button.hpp"
Button sw1(0); //IO0


//writes all buf bytes to client- blocking
//(client.write may not write all bytes, so created this blocking function)
void client_writer(WiFiClient& client, const uint8_t* buf, size_t len)
{
    size_t sent = 0;
    while(sent < len) sent += client.write(&buf[sent], len-sent);
}
//same, for const char* (so caller doesn't have to cast)
void client_writer(WiFiClient& client, const char* buf, size_t len){
    client_writer(client, (const uint8_t*)buf, len);
}
//now 0 terminated buffer versions
void client_writer(WiFiClient& client, const uint8_t* buf){
    size_t len = strlen((const char*)buf);
    client_writer(client, buf, len);
}
void client_writer(WiFiClient& client, const char* buf){
    client_writer(client, (const uint8_t*)buf);
}

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
            if(len) client_writer(client, buf, len);

            //file: esp32-hal-uart.c
            //in /home/owner/.arduino15/packages/esp32/hardware/esp32/1.0.0/cores/esp32/
            //changed
            //while(uart->dev->status.rxfifo_cnt) {
            //to
            //while(uart->dev->mem_rx_status.wr_addr != uart->dev->mem_rx_status.rd_addr) {
            //to fix rxfifo_cnt sometimes reading as 0 when there are bytes available
    }

}

void boot2ap(WiFiClient& client)
{
    //"boot2ap"
    uint8_t buf[256];
    WifiCredentials wifidata;
    bool b = wifidata.boot2ap();
    snprintf((char*)buf, 256, "boot2ap: %d\n", b);
    client_writer(client, buf);
}

void list_credentials(WiFiClient& client)
{
    uint8_t buf[256];
    WifiCredentials wifidata;
    snprintf((char*)buf, 256, "[slot][ssid][pass]\n");
    client_writer(client, buf);
    for( uint8_t i = 0; i < wifidata.maxn(); i++ ){
        String s = wifidata.get_ssid(i);
        String p = wifidata.get_pass(i);
        snprintf((char*)buf, 256, "[%2d][%s][%s]\n", i, &s[0], &p[0]);
        client_writer(client, buf);
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
            client_writer(client, "missing index or index not valid\n");
            return;
        }
    }
    if(idx > wifidata.maxn()){
        client_writer(client, "index is out of range\n");
        return;
    }
    wifidata.put_ssid(idx, "");
    wifidata.put_pass(idx, "");
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
            client_writer(client, "missing index or index not valid\n");
            return;
        }
    }
    if(idx > wifidata.maxn()){
        client_writer(client, "index is out of range\n");
        return;
    }

    int si = s.indexOf("ssid=");
    if(si > 0){
        wifidata.put_ssid(idx, &s[si+5]);
        return;
    }

    si = s.indexOf("pass=");
    if(si > 0){
        wifidata.put_pass(idx, &s[si+5]);
        return;
    }
    client_writer(client, "did not find ssid= or pass=\n");
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
    else { client_writer(client, "invalid command\n"); }
    return true;
}

//handler for info telnet connection
void telnet_info_handler(WiFiClient& client, TelnetServer::msg_t msg)
{
    static uint8_t buf[128];
    static uint8_t idx;
    switch(msg){
        case TelnetServer::START:
            client_writer(client, "Connected to info port\n\n$ ");
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
                //client_writer(client, &buf[idx], len); //echo back
                idx += len; //increase index by length
                buf[idx] = 0; //0 terminate
                if(check_cmd(client, buf, sizeof(buf))){
                    idx = 0; //if \r or \n found, can reset index
                    client_writer(client, "$ ");
                }
                if(idx >= sizeof(buf)-1){
                    client_writer(client, "\n\ncommand too long :(\n\n$ "); //command buffer overflow
                    idx = 0; //start over
                }
            }

    }
}

//create telnet server for both
TelnetServer telnet_info(2300, "info", telnet_info_handler);
TelnetServer telnet_uart(2302, "uart2", telnet_uart_handler);

void ap_mode(){
    Serial.printf("\nstarting access point...");
    WiFi.softAP("SNAP-esp32");
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
        String s = wifidata.get_ssid(i);
        String p = wifidata.get_pass(i);
        if(not s.length()) continue; //if ssid blank, skip
        printf("using credentials from nvs storage...\n  %s  %s\n", &s[0], &p[0]);
        wifiMulti.addAP(&s[0], &p[0]);
        found1 = true;
    }
    if(not found1){
        Serial.printf("no credentials found in nvs storage\n");
        ap_mode();
    }
    Serial.printf("\n\n");

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
        while(sw1.pressed() > 3000);
        WifiCredentials wifidata;
        wifidata.boot2ap(true);
        ESP.restart();
    }
}
