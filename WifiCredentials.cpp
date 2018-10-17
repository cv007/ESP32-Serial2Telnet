#include "WifiCredentials.hpp"

WifiCredentials::WifiCredentials()
{
    m_credentials.begin("credentials", false); //false = r/w
}

uint8_t WifiCredentials::maxn()
{
    return m_maxn;
}

String WifiCredentials::get_ssid(uint8_t idx)
{
    if(idx > m_maxn) return {};
    String s("ssid" + String(idx));
    return m_credentials.getString(s.c_str(), {});
}
String WifiCredentials::get_pass(uint8_t idx)
{
    if(idx > m_maxn) return {};
    String s("pass" + String(idx));
    return m_credentials.getString(s.c_str(), {});
}

size_t WifiCredentials::put_ssid(uint8_t idx, String ssid)
{
    if(ssid.length() > 31 || idx > m_maxn) return 0;
    return m_credentials.putString(String("ssid" + String(idx)).c_str(), ssid);
}
size_t WifiCredentials::put_pass(uint8_t idx, String pass)
{
    if(pass.length() > 63 || idx > m_maxn) return 0;
    return m_credentials.putString(String("pass" + String(idx)).c_str(), pass);
}

bool WifiCredentials::clear()
{
    return m_credentials.clear();
}

bool WifiCredentials::boot2ap()
{
    return m_credentials.getBool("bootAP",0);
}
void WifiCredentials::boot2ap(bool tf)
{
    if(boot2ap() == tf) return; //no change, no need to write
    m_credentials.putBool("bootAP", tf);
}
