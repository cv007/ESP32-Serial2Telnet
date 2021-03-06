#include "NvsSettings.hpp"
#include <WiFi.h>

//default names if not set yet
const String hostname_default = "SNAP";
const String APname_default = "SNAP-AP";
//default uart2 baud
const uint32_t uart2baud_default = 115200;


NvsSettings::NvsSettings()
{
    m_settings.begin("settings");
}

uint8_t NvsSettings::wifimaxn()
{
    return m_wifimaxn;
}

//private
String NvsSettings::gets(String s, String alt = String())
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
    return gets(String("ssid" + String(idx)));
}
size_t NvsSettings::ssid(uint8_t idx, String ssid)
{
    if(ssid.length() > 31 || idx > m_wifimaxn) return 0;
    return puts(String("ssid" + String(idx)), ssid);
}

String NvsSettings::pass(uint8_t idx)
{
    if(idx > m_wifimaxn) return {};
    return gets(String("pass" + String(idx)));
}
size_t NvsSettings::pass(uint8_t idx, String pass)
{
    if(pass.length() > 63 || idx > m_wifimaxn) return 0;
    return puts(String("pass" + String(idx)), pass);
}

String NvsSettings::hostname()
{
    String s = gets(String("hostname"));
    if(s.length()) return s;
    return hostname_default;
}
size_t NvsSettings::hostname(String s)
{
    return puts(String("hostname"), s);
}

String NvsSettings::APname()
{
    String s = gets(String("APname"));
    if(s.length()) return s;
    return APname_default;
}
size_t NvsSettings::APname(String s)
{
    return puts(String("APname"), s);
}

uint32_t NvsSettings::uart2baud()
{
    return m_settings.getUInt("uart2baud", uart2baud_default);
}
size_t NvsSettings::uart2baud(uint32_t baud)
{
    return m_settings.putUInt("uart2baud", baud);
}

bool NvsSettings::clear()
{
    return m_settings.clear();
}

bool NvsSettings::boot_to_AP()
{
    return m_settings.getBool("boot", false);
}
size_t NvsSettings::boot_to_AP(bool tf)
{
    return m_settings.putBool("boot", tf);
}

bool NvsSettings::erase_all()
{
    return m_settings.clear();
}
