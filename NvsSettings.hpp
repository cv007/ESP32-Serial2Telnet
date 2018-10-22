#pragma once

#include <Preferences.h>

// max ssid size = 31, max pass size = 63
// store ssid 0-m_wifimaxn, pass 0-m_wifimaxn
// store hostname, APname, boot2ap

struct NvsSettings {

    NvsSettings();
    String ssid(uint8_t);           //index -> ssid string (empty if none)
    String pass(uint8_t);           //index -> pass string (empty if none)
    size_t ssid(uint8_t, String);   //index, ssid -> bytes written
    size_t pass(uint8_t, String);   //index, pass -> bytes written

    String hostname();
    size_t hostname(String);

    String APname();
    size_t APname(String s);

    uint8_t wifimaxn();             //-> max number of wifi credentials can store

    bool clear();                   //clear all nvs entries for this namespace

    enum : bool { STA, AP };
    bool boot();                    //get value to run AP or STA
    size_t boot(bool);              //set value to run AP or STA

    bool debug();                   //get debug value
    size_t debug(bool);             //set debug value, 0=no uart0 output 1=use uart0 as debug ouput
                                    //if 0, use telnet (2300) to view info via debug command
                                    //  uart1 will be debug port, tx-loopback-rx,
                                    //  then telnet function will receive uart1 data if debug command run
                                    //if 1, debug output will go out uart0 as normal
                                    //  and uart0 serial-telnet will be disabled

    bool erase_all();               //erase all data in this namespace


    private:
    String gets(String, String);
    size_t puts(String, String);
    Preferences m_settings;
    const uint8_t m_wifimaxn = 8;

};


