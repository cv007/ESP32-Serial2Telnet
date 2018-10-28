#pragma once

#include <Preferences.h>

// max ssid size = 31, max pass size = 63
// store ssid 0-m_wifimaxn, pass 0-m_wifimaxn
// store hostname, APname, boot

struct NvsSettings {

    NvsSettings();
    String ssid(uint8_t);           //index -> ssid string (empty if none)
    size_t ssid(uint8_t, String);   //index, ssid -> bytes written

    String pass(uint8_t);           //index -> pass string (empty if none)
    size_t pass(uint8_t, String);   //index, pass -> bytes written

    String hostname();              //get hostname (used in STA mode)
    size_t hostname(String);        //set hostname

    String APname();                //get access point name
    size_t APname(String s);        //set acces point name (used in AP mode)

    uint8_t wifimaxn();             //-> max number of wifi credentials can store

    bool clear();                   //clear all nvs entries for this namespace

    using boot_t = enum : bool { STA, AP };
    bool boot();                    //get value to run AP or STA
    size_t boot(boot_t);            //set value to run AP or STA

    bool erase_all();               //erase all data in this namespace


    private:

    String gets(String, String);
    size_t puts(String, String);
    Preferences m_settings;
    const uint8_t m_wifimaxn = 8;   //limit to 8 ssid/pass

};


