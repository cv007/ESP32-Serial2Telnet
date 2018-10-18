#pragma once

#include <Preferences.h>

// max ssid size = 31, max pass size = 63
// store ssid 0-m_maxn, pass 0-m_maxn
// store hostname, APname, boot2ap

struct WifiCredentials {

    WifiCredentials();
    String ssid(uint8_t);           //index -> ssid string (empty if none)
    String pass(uint8_t);           //index -> pass string (empty if none)
    size_t ssid(uint8_t, String);   //index, ssid -> bytes written
    size_t pass(uint8_t, String);   //index, pass -> bytes written
    String hostname();
    size_t hostname(String);
    String APname();
    size_t APname(String s);
    uint8_t maxn();                     //-> max number of wifi credentials can store
    bool clear();                       //clear all nvs entries for this namespace
    bool boot2ap();                     //get value to run AP or STA
    void boot2ap(bool);                 //set value to run AP or STA


    private:
    String gets(String, String);
    size_t puts(String, String);
    Preferences m_credentials;
    const uint8_t m_maxn = 8;

};


