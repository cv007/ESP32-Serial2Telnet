#include "WifiCredentials.hpp"

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
