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
    String get_wifi(uint8_t);
    size_t put_wifi(String ssid, String pass);
    uint8_t index();
    bool clear();

    private:
    Preferences m_credentials;
    uint8_t m_index;
    const uint8_t m_maxn = 8;
    void next_index();
};

WifiCredentials::WifiCredentials()
{
    m_credentials.begin("credentials", false); //false = r/w
    m_index = m_credentials.getUInt("index",0);
}

uint8_t WifiCredentials::index()
{
    return m_index;
}

void WifiCredentials::next_index()
{
    if(++m_index > m_maxn) m_index = 0;
    m_credentials.putUInt("index", m_index);
}

String WifiCredentials::get_wifi(uint8_t idx)
{
    if(idx > m_maxn) return {};
    String s("wifi" + String(idx));
    return m_credentials.getString(s.c_str(), {});
}

size_t WifiCredentials::put_wifi(String ssid, String pass)
{
    if(ssid.length() > 31 || pass.length() > 63) return 0;
    size_t ret = m_credentials.putString(String("wifi" + String(m_index)).c_str(), String(ssid + '\t' + pass));
    if(ret) next_index();
    return ret;
}

bool WifiCredentials::clear()
{
    m_index = 0;
    return m_credentials.clear();
}
