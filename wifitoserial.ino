/*
  UART to Telnet Server for ESP32
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



//writes all buf bytes to client- blocking
//(client.write may not write all bytes, so created this blocking function)
void client_writer(WiFiClient& client, const uint8_t* buf, size_t len)
{
    size_t sent = 0;
    while(sent < len){
        sent += client.write(&buf[sent], len-sent);
    }
}
//same, for const char* (so caller doesn't have to cast)
void client_writer(WiFiClient& client, const char* buf, size_t len){
    client_writer(client, (const uint8_t*)buf, len);
}


//handler for uart telnet connection
void telnet_uart_handler(WiFiClient& client, bool init)
{
    if(init){
        //flush serial when first connected
        Serial2.flush();
        //flush does not seem to work on rx
        //so just keep getting rx data until no more available
        while(Serial2.available()) Serial2.read();
        return;
    }
    size_t len;
    uint8_t buf[128];
    //check client for data
    //get data from the telnet client and push it to the UART
    //(Serial2.write is blocking, will complete)
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
//while(uart->dev->status.rxfifo_cnt || (uart->dev->mem_rx_status.wr_addr != uart->dev->mem_rx_status.rd_addr)) {



//Serial2.available() seems to return 0 when there are bytes in queue
//the bytes are not lost and will still show up, but the queue counter seems to
//somehoe get corrupted (maybe 1 count an hour or 2)
//     len = Serial2.available();
//     if(len){
//         if(len > 128) len = 128;
//         Serial2.readBytes(buf, len);
//         client_writer(buf, len);
//     }

//this works
//     if(Serial2.available()){
//         size_t n = Serial2.readBytes(buf, 128);
//         client_writer(client, buf, n);
//     }

//trying this
    //read() returns -1 if no bytes available
    //get up to 128bytes
    //(no timeout involved)
//     for( len=0;len<128;len++ ){
//         int r = Serial2.read();
//         if(r<0) break;
//         buf[len]=r;
//     }
//     //if any bytes, send them out telnet
//     if(len) client_writer(client, buf, len);

}

//handler for info telnet connection
void telnet_info_handler(WiFiClient& client, bool init)
{
    if(init) {
        client_writer(client, "Connected to info port\n\n", 24);
        return;
    }
    //check clients for data
    //get data from the telnet client and echo back
    while(client.available()){
        uint8_t b = client.read();
        client_writer(client, &b, 1);
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
    Serial2.begin(230400);
Serial2.setTimeout(1); //set timeout for reads
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
}
