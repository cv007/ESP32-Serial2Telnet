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
    String get_wifi(uint8_t);           //index -> credential string (empty if none)
    size_t put_wifi(String, String);    //ssid,pass -> bytes written
    uint8_t index();                    //-> next index to write ("wifiN", N=index)
    bool clear();                       //clear all nvs entries for this namespace

    private:
    Preferences m_credentials;
    uint8_t m_index;
    const uint8_t m_maxn = 8;
    void next_index();
};


