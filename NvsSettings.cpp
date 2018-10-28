#include "NvsSettings.hpp"


NvsSettings::NvsSettings()
{
    m_settings.begin("settings");
}

uint8_t NvsSettings::wifimaxn()
{
    return m_wifimaxn;
}

//private
String NvsSettings::gets(String s, String alt)
{
    return m_settings.getString(s.c_str(), alt);
}
size_t NvsSettings::puts(String key, String s)
{
    return m_settings.putString(key.c_str(), s);
}
//


String NvsSettings::ssid(uint8_t idx)
{
    if(idx > m_wifimaxn) return {};
    return gets(String("ssid" + String(idx)), {});
}
size_t NvsSettings::ssid(uint8_t idx, String ssid)
{
    if(ssid.length() > 31 || idx > m_wifimaxn) return 0;
    return puts(String("ssid" + String(idx)), ssid);
}

String NvsSettings::pass(uint8_t idx)
{
    if(idx > m_wifimaxn) return {};
    return gets(String("pass" + String(idx)), {});
}
size_t NvsSettings::pass(uint8_t idx, String pass)
{
    if(pass.length() > 63 || idx > m_wifimaxn) return 0;
    return puts(String("pass" + String(idx)), pass);
}

String NvsSettings::hostname(){         return gets(String("hostname"), {}); }
size_t NvsSettings::hostname(String s){ return puts(String("hostname"), s); }

String NvsSettings::APname(){           return gets(String("APname"), {}); }
size_t NvsSettings::APname(String s){   return puts(String("APname"), s); }

bool NvsSettings::clear(){              return m_settings.clear(); }

bool NvsSettings::boot(){               return m_settings.getBool("boot", STA); }
size_t NvsSettings::boot(boot_t tf){    return m_settings.putBool("boot", tf); }

bool NvsSettings::erase_all(){ return m_settings.clear(); }
