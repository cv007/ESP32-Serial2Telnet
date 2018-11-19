/*
  UART to Telnet Server for ESP32

  lights-
    green - on = connected to wifi
            blinking slow = trying to connect to wifi
            blinkng fast = access point mode
            fast blink 2sec, off 2sec, repeat = boot to ap mode triggered (now release boot switch)

  buttons-
    sw_reset    reset switch
    sw_boot     boot switch
                -to go into access point mode, press/hold boot switch
                -to go into bootloader mode, press/hold boot switch, press reset switch

  connections-
    uart0 - 3pin connector (used for esp32 programming, debug output)
    uart2 - programming connector


  initial setup- (initial firmware loaded)
        power on
            delay 5 seconds in deep sleep (give time for SNAP to enumerate)

            if boot flag on OR no wifi credentials stored-
                start access point mode with telnet info port 2300 enabled
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
#include <rom/rtc.h> //reset reason


#include "TelnetServer.hpp"
#include "NvsSettings.hpp"
#include "Commander.hpp"
#include "WebServer.hpp"


//sw_boot (IO0) long press = run wifi access point
#include "Button.hpp"
Button sw_boot(0);

//status led (IO23)
#include "LedStatus.hpp"
LedStatus led_wifi(23);


WiFiMulti wifiMulti;

//create telnet servers
TelnetServer telnet_info(2300, "info", TelnetServer::INFO);
TelnetServer telnet_uart2(2302, "uart2", TelnetServer::SERIAL2);


//restart whenever wifi connect problem
void restart()
{
    Serial.printf("wifi connect failed, restarting in 10 seconds...\n\n");
    delay(10000);
    ESP.restart(); //just reboot and start over
}

//wifi connect, n attempts (n seconds)
//if cannot connect, reset
void wifi_connect(int n)
{
    //set hostname
    //(setHostname code modified, so can set any time)
    //(any reconnect will get latest hostname from settings,
    // so will see new setting if disconnect/reconnect)
    NvsSettings settings;
    WiFi.setHostname(settings.hostname().c_str());

    led_wifi.slow();

    for(; ; delay(1000), n--){
        if(n == 0) restart();
        Serial.printf("connecting wifi...%d\n", n);
        if(wifiMulti.run() == WL_CONNECTED){
            Serial.printf("connected to SSID: %s\nclient IP: %s\nhostname: %s\n\n",
                WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), settings.hostname().c_str()
            );
            break;
        }
    }
    led_wifi.on();
}

//start access point if no wifi settings, or boot mode set
//start telnet info server port 2300
//start web server port 80 (can use browser to enter commands)
//to access- telnet 192.168.4.1 2300
//then run commands (mainly to set wifi credentials)
//port 2302 not active
void ap_mode(){

    //led blink fast in AP mode
    led_wifi.fast();

    Serial.printf("\n== starting access point mode ==");

    NvsSettings settings;
    //back to STA for next boot
    settings.boot_to_AP(false);
    WiFi.softAP(settings.APname().c_str());
    Serial.printf("access point ip address: %s\n\n",WiFi.softAPIP().toString().c_str());

    delay(2000);

    TelnetServer telnet_ap(2300, "info", TelnetServer::INFO);
    WebServer web_server(80, "http");

    telnet_ap.start();
    web_server.start();

    for(;;){
        telnet_ap.check();
        web_server.check();
    }

    //to exit AP mode
    //can use 'sys reboot' command from telnet/web,
    //or simply use reset button

}


//one time setup
void setup()
{
    //delay a little before starting (5 sec)
    //(keep power down while usb enumerating when powered by SNAP)
    //deepSleep any time we didn't just wake from deepSleep
    //(any reset cause except deepSleep)
    if(rtc_get_reset_reason(0) !=  DEEPSLEEP_RESET) ESP.deepSleep(5 * 1000000);

    //debug ouput
    Serial.begin(115200);

    //led initially on (to show alive)
    //wifi_connect() will take care of the rest
    //(but will end up in wifi_connect() soon, so probably not worht it)
    led_wifi.on();

    //if boot mode set to AP, run access point
    NvsSettings settings;
    if(settings.boot_to_AP()) ap_mode();

    //STA mode
    Serial.printf("\n== starting station mode ==\n");

    //add stored wifi credentials, if none found goto AP mode
    uint8_t maxn = settings.wifimaxn();
    for( uint8_t i = 0, n = 0; i < maxn; ){
        if(wifiMulti.addAP(settings.ssid(i).c_str(), settings.pass(i).c_str())){
            Serial.printf("added wifi credentials [%d] from nvs storage\n", i);
            n++;
        }
        if(++i == maxn and n == 0){
            Serial.printf("no wifi credentials found, switching to AP mode\n");
            ap_mode();
        }
    }

    //10 attempts
    wifi_connect(10);

    //telnet servers report address as 0.0.0.0 sometimes even though they
    //are correct ip (obtained via dhcp), a little delay may help- now seems ok so far
    delay(2000);

    //start the servers
    telnet_info.start();
    telnet_uart2.start();


    /*
    //just quickly testing some code for debugging output
    //
    static bool _init;
    static uint32_t hash;
    uint32_t myvar = 0x12345678;
    //one time- give receiver a hash and other info associated with var
    if(not _init){
        std::string s = __FILE__; s += __FUNCTION__;
        _hash = std::hash<std::string>{}(s);
        Serial.printf(":%08X:%s:%s:%u:%s\n",hash,__FILE__,__FUNCTION__,__LINE__,"myvar");
    }
    //use hash to identify var, send out info
    //receiver will know which var by the hash
    Serial.printf(":%08X:%08X\n",hash,myvar);
    //this could be binary to reduce size
    //<hash><cp0count><uint32_t>
    */
}



//main loop
void loop()
{
    //check if connection lost
    if(wifiMulti.run() != WL_CONNECTED){
        Serial.printf("wifi connection lost, attempting to reconnect...\n");
        //try for 20 times (1 second interval), if failed just reset esp
        wifi_connect(20);
    }

    //let each server check client connections/data
    telnet_info.check();
    telnet_uart2.check();

    //check switch - long press to go into AP mode
    if(sw_boot.long_press()){
        Serial.printf("BOOT switch long press, booting into AP mode...\n");
        telnet_info.stop();
        telnet_uart2.stop();
        NvsSettings settings;
        settings.boot_to_AP(true);
        //led blink fast 2sec, then off 2sec
        //wait for release so sw is not pressed when rebooted
        //which would then boot into bootloader
        uint8_t t = 0;
        while(t++, delay(100), sw_boot.down()){
            if(t == 1) led_wifi.fast();   //1-20 (2sec) = fast
            if(t == 21) led_wifi.off();   //21-40 (2sec) = off
            if(t > 40) t = 0;             //start over

        }
        delay(100); //debounce time up
        ESP.restart();
    }
}
