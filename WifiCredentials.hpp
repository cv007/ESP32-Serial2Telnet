#pragma once

#include <Preferences.h>

//max ssid size = 31, max pass size = 63
// "index"  = UInt -> next index where credentials will be stored 0-m_maxn
// overwite the oldest credential when adding to full set of credentials
// stored in keys "wifi0" - "wifiN" where N = m_maxn
// ssid and password stored as one string, separated by tab char
// (characters below space (' '/32/0x20) supposed to be off limits, so tab should be ok)
// (just want to store both in one write- fails or succedes)
// wifi0 = "ssid\tpassword"
// wifi1 = "ssid\tpassword"
// ...
// up to m_maxn can be stored (really don't need that many)

struct WifiCredentials {
    WifiCredentials();
    String get_ssid(uint8_t);           //index -> ssid string (empty if none)
    String get_pass(uint8_t);           //index -> pass string (empty if none)
    size_t put_ssid(uint8_t, String);   //index, ssid -> bytes written
    size_t put_pass(uint8_t, String);   //index, pass -> bytes written
    uint8_t maxn();                     //-> max number of wifi credentials can store
    bool clear();                       //clear all nvs entries for this namespace
    bool boot2ap();                     //get value to run AP or STA
    void boot2ap(bool);                 //set value to run AP or STA

    private:
    Preferences m_credentials;
    const uint8_t m_maxn = 8;
};


