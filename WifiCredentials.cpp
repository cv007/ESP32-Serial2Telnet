#include "WifiCredentials.hpp"

WifiCredentials::WifiCredentials()
{
    m_credentials.begin("credentials", false); //false = r/w
}

uint8_t WifiCredentials::maxn()
{
    return m_maxn;
}

//private
String WifiCredentials::gets(String s, String alt)
{
    return m_credentials.getString(s.c_str(), alt);
}
size_t WifiCredentials::puts(String key, String s)
{
    return m_credentials.putString(key.c_str(), s);
}
//

String WifiCredentials::ssid(uint8_t idx)
{
    if(idx > m_maxn) return {};
    return gets(String("ssid" + String(idx)), {});
}
String WifiCredentials::pass(uint8_t idx)
{
    if(idx > m_maxn) return {};
    return gets(String("pass" + String(idx)), {});
}

size_t WifiCredentials::ssid(uint8_t idx, String ssid)
{
    if(ssid.length() > 31 || idx > m_maxn) return 0;
    return puts(String("ssid" + String(idx)), ssid);
}
size_t WifiCredentials::pass(uint8_t idx, String pass)
{
    if(pass.length() > 63 || idx > m_maxn) return 0;
    return puts(String("pass" + String(idx)), pass);
}

String WifiCredentials::hostname(){         return gets(String("hostname"), {}); }
size_t WifiCredentials::hostname(String s){ return puts(String("hostname"), s); }
String WifiCredentials::APname(){           return gets(String("APname"), {}); }
size_t WifiCredentials::APname(String s){   return puts(String("APname"), s); }
bool WifiCredentials::clear(){              return m_credentials.clear(); }
bool WifiCredentials::boot(){               return m_credentials.getBool("boot", STA); }
void WifiCredentials::boot(bool tf)
{
    if(boot() == tf) return; //no change, no need to write
    m_credentials.putBool("boot", tf);
}
bool WifiCredentials::erase_all(){ return m_credentials.clear(); }
