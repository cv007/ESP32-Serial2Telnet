/*
  UART to Telnet Server for ESP32

  lights-
    green - on = connected to wifi
            blinking slow = trying to connect to wifi
            blinkng fast = access point mode
            5 blinks, then off = boot to ap mode triggered (now release boot switch)

  buttons-
    sw_reset    reset switch
    sw_boot     boot switch
                -to go into access point mode, press/hold boot switch
                -to go into bootloader mode, press/hold boot switch, press reset switch

  connectoions-
    uart0 - programming connector (used for esp32 programming, debug output)
    uart2 - programming connector


  initial setup- (initial firmware loaded)
        power on
            delay 5 seconds

            if boot flag on OR no wifi credentials stored-
                start access point mode with only telnet info port 2300 enabled
                boot flag is cleared

            try to connect to available wifi access points using stored credentials
                if unable, keep trying for a period of time, then reset
                telnet port 2300 = info
                telnet port 2302 = uart2

            if boot switch pressed >3 sec, reboot to access point mode
                after releasing switch, the esp will reboot

            if in access point mode-
                can connect to info on port 2300 to enter wifi credentials using console commands
                reboot via reset switch (or use reboot command via telnet)




  console commands- see "Commander.hpp"



*/
#include <WiFi.h>
#include <WiFiMulti.h>

#include "TelnetServer.hpp"
#include "NvsSettings.hpp"
#include "Commander.hpp"


//sw_boot (IO0) long press = run wifi access point
#include "Button.hpp"
Button sw_boot(0);

//status led (IO23)
#include "LedStatus.hpp"
LedStatus led_wifi;


WiFiMulti wifiMulti;

//create telnet servers
TelnetServer telnet_info(2300, "info", TelnetServer::INFO);
TelnetServer telnet_uart2(2302, "uart2", TelnetServer::SERIAL2);




//start access point, start telnet info server port 2300
//to access- telnet 192.168.4.1 2300
//then run commands (mainly to set wifi credentials)
//port 2302 not active
void ap_mode(){
    //led blink fast
    led_wifi.fast();

    Serial.printf("\n");
    for(int i = 5; i > 0; Serial.printf("startup delay %d\n", i), delay(1000), i--);
    Serial.printf("\nstarting access point...");

    NvsSettings settings;
    String APname = settings.APname();
    if(APname.length() == 0) APname = "SNAP-AP";

    WiFi.softAP(APname.c_str());
    TelnetServer telnet_ap(2300, "info", TelnetServer::INFO);
    Serial.printf("%s\n\n",WiFi.softAPIP().toString().c_str());

    delay(5000);
    telnet_ap.start();
    for(;;){
        telnet_ap.check();
    }
}


//one time setup
void setup()
{
    //start led status, default settings (IO23)
    led_wifi.init();

    //debug ouput
    Serial.begin(115200);

    //if boot mode set to AP, run access point
    NvsSettings settings;
    if(settings.boot() == settings.AP){
        //was flagged to boot to AP mode
        //back to STA for next boot
        settings.boot(settings.STA);
        //start access point
        ap_mode();
    }

    Serial.printf("\n");
    for(int i = 5; i > 0; Serial.printf("startup delay %d\n", i), delay(1000), i--);
    Serial.printf("\n");

    bool found1 = false;
    for( uint8_t i = 0; i < settings.wifimaxn(); i++ ){
        String s = settings.ssid(i);
        String p = settings.pass(i);
        if(not s.length()) continue; //if ssid blank, skip
        wifiMulti.addAP(&s[0], &p[0]);
        Serial.printf("adding wifi credentials [%d] from nvs storage\n", i);
        found1 = true;
    }
    if(not found1){
        Serial.printf("no wifi credentials found, switching to AP mode\n");
        ap_mode();
    }

    //set hostname if stored
    String hn = settings.hostname();
    if(hn.length()){
        Serial.printf("setting hostname to [%s]\n", hn.c_str());
        WiFi.setHostname(hn.c_str());
    }

    //led blink slow
    led_wifi.slow();

    for(int i = 10; i > 0; delay(1000), i--){
        Serial.printf("connecting wifi...%d\n", i);
        if(wifiMulti.run() == WL_CONNECTED) break;
    }

    if(wifiMulti.run() == WL_CONNECTED){
        Serial.printf("connected to SSID: %s   client IP: %s\n\n",
            WiFi.SSID().c_str(), WiFi.localIP().toString().c_str()
        );
    } else {
        Serial.printf("connect failed, restarting in 10 seconds...\n\n");
        delay(10000);
        ESP.restart(); //for now, just reboot and start over
    }

    delay(3000);
    //led on - is connected
    led_wifi.on();
    //start the servers
    telnet_info.start();
    telnet_uart2.start();
}

//main loop
void loop()
{
    //check if connection lost
    if(wifiMulti.run() != WL_CONNECTED){
        led_wifi.slow();
        Serial.printf("wifi connection lost, attempting to reconnect...\n");
        //try for 20 times (1 second interval), if failed just reset esp)
        for(auto i = 20; ; delay(1000), i--){
            Serial.printf("connecting wifi...%d\n", i);
            if(wifiMulti.run() == WL_CONNECTED){
                led_wifi.on();
                break;
            }
            if(i == 0){
                Serial.printf("connect failed, restarting in 10 seconds...\n\n");
                delay(10000);
                ESP.restart();
            }
        }
        Serial.printf("connected to SSID: %s   client IP: %s\n\n",
            WiFi.SSID().c_str(), WiFi.localIP().toString().c_str()
        );
    }

    //let each server check client connections/data
    telnet_info.check();
    telnet_uart2.check();


    //check switch
    if(sw_boot.long_press()){
        Serial.printf("BOOT switch long press, booting into AP mode...\n");
        telnet_info.stop();
        telnet_uart2.stop();
        NvsSettings settings;
        settings.boot(settings.AP);
        //led blink fast 2sec, then off 2sec
        //wait for release so sw is not pressed when rebooted
        //which would then boot into bootloader
        uint8_t t = 0;
        while(t++, delay(100), sw_boot.down()){
            if(t == 1) led_wifi.fast();   //1-20 (2sec) = fast
            if(t > 20) led_wifi.off();    //21-40 (2sec) = off
            if(t > 40) t = 0;               //start over

        }
        ESP.restart();
    }
}
