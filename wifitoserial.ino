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
            check for wifi data, if none, goto access point mode
            if wifi data, try to connect
            if sw2 pressed >3 sec, goto access point mode




*/
#include <WiFi.h>
#include <WiFiMulti.h>

#include "TelnetServer.hpp"

//connection list info to add
typedef struct {
    const char* ssid;
    const char* password;
} wifi_credentials_t;

//my connection list (add more as needed)
wifi_credentials_t wifi_credentials[] = {
    #include "mycredentials.h" //file ignored by git
    //{ "ssid", "password" }, ... (1 or more)
};

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

//handler for info telnet connection
void telnet_info_handler(WiFiClient& client, TelnetServer::msg_t msg)
{
    switch(msg){
        case TelnetServer::START:
            client_writer(client, "Connected to info port\n\n", 24);
            break;
        case TelnetServer::STOP:
            break;
        case TelnetServer::CHECK:
            //check clients for data
            //get data from the telnet client and echo back
            while(client.available()){
                uint8_t b = client.read();
                client_writer(client, &b, 1);
            }
    }
}

//create telnet server for both
TelnetServer telnet_info(2323, "info", telnet_info_handler);
TelnetServer telnet_uart(23, "uart", telnet_uart_handler);


//one time setup
void setup()
{
    Serial.begin(115200);
    Serial.printf("\nStartup delay... ");
    for(int i = 5; i > 0; Serial.printf("%d ", i), delay(1000), i--);
    Serial.printf("\n\n");

    for(auto w : wifi_credentials){
        wifiMulti.addAP(w.ssid, w.password);
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

    //start UART and the servers
//     Serial2.begin(230400);
//     Serial2.setTimeout(0); //set timeout for reads
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
    if(p > 3000) Serial.printf("BOOT switch long press %d ms\n",p);
}
