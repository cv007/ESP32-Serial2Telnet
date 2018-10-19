/*
  UART to Telnet Server for ESP32

  lights-
    green - on = connected to wifi
            blinking slow = trying to connect to wifi
            blinkng fast = access point mode

  buttons-
    reset_sw    reset switch (case protruding)
    boot_sw     boot switch (case recessed)
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




  console commands- see "Commander.hpp"



*/
#include <WiFi.h>
#include <WiFiMulti.h>

#include "TelnetServer.hpp"
#include "WifiCredentials.hpp"
#include "Commander.hpp"

//boot_sw (IO0) long press = run wifi access point
#include "Button.hpp"

WiFiMulti wifiMulti;

//create telnet servers
TelnetServer telnet_info(2300, "info", TelnetServer::INFO);
TelnetServer telnet_uart(2302, "uart2", TelnetServer::SERIAL2);

//start access point, telnet server port 2300
//to access-
//telnet 192.168.4.1 2300
//then run commands (mainly to set wifi credentials)
void ap_mode(){
    WifiCredentials wifidata;
    Serial.printf("\nstarting access point...");
    String APname = wifidata.APname();
    if(APname.length() == 0) APname = "SNAP-AP";
    WiFi.softAP(APname.c_str());
    //TelnetServer telnet_ap(2300, "apinfo", telnet_info_handler, TelnetServer::INFO);
    TelnetServer telnet_ap(2300, "apinfo", TelnetServer::INFO);
    Serial.printf("%s\n\n",WiFi.softAPIP().toString().c_str());
    delay(5000);
    telnet_ap.start();
    for(;;){
        telnet_ap.check();
        //check switch
        if(boot_sw.long_press()){
            Serial.printf("BOOT switch pressed, rebooting...\n");
            delay(2000);
            while(boot_sw.down());
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
    if(wifidata.boot() == wifidata.AP){
        //was flagged to boot to AP mode
        wifidata.boot(wifidata.STA); //back to STA for next boot
        //start access point
        ap_mode();
    }

    bool found1 = false;
    for( uint8_t i = 0; i < wifidata.maxn(); i++ ){
        String s = wifidata.ssid(i);
        String p = wifidata.pass(i);
        if(not s.length()) continue; //if ssid blank, skip
        printf("adding wifi credentials from nvs storage...\n  %s  %s\n", &s[0], &p[0]);
        wifiMulti.addAP(&s[0], &p[0]);
        found1 = true;
    }
    if(not found1){
        Serial.printf("no wifi credentials found in nvs storage\n");
        ap_mode();
    }
    Serial.printf("\n");

    //set hostname if stored
    String hn = wifidata.hostname();
    if(hn.length()){
        Serial.printf("setting hostname...%s\n\n", hn.c_str());
        WiFi.setHostname(hn.c_str());
    }

    Serial.printf("connecting wifi...");
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
    if(boot_sw.long_press()){
        Serial.printf("BOOT switch long press\n");
        telnet_info.stop();
        telnet_uart.stop();
        WifiCredentials wifidata;
        wifidata.boot(wifidata.AP);
        //wait for release so sw is not pressed when rebooted
        //which would then boot into bootloader
        while(boot_sw.down());
        ESP.restart();
    }
}
