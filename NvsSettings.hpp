#pragma once

#include <Preferences.h>

// max ssid size = 31, max pass size = 63
// store ssid 0-m_wifimaxn, pass 0-m_wifimaxn
// store hostname, APname, boot, uart2baud

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

    uint32_t uart2baud();           //get uart2 baud
    size_t uart2baud(uint32_t);     //set uart2 baud

    uint8_t wifimaxn();             //-> max number of wifi credentials can store

    bool clear();                   //clear all nvs entries for this namespace

    bool boot_to_AP();              //get boot val, 1=boot to AP mode
    size_t boot_to_AP(bool);        //set boot val, 1=AP, 0=STA

    bool erase_all();               //erase all data in this namespace


    private:

    String gets(String, String);
    size_t puts(String, String);
    Preferences m_settings;
    const uint8_t m_wifimaxn = 8;   //limit to 8 ssid/pass

};


