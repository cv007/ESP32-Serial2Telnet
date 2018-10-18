#include "Commander.hpp"
#include "WifiCredentials.hpp"

//=============================================================================
// root commands
//=============================================================================
static void sys(WiFiClient&, String);
static void wifi(WiFiClient&, String);
static void net(WiFiClient&, String);

//=============================================================================
// root command list - name:function
//=============================================================================
typedef void(*cmdfunc_t)(WiFiClient&, String);
typedef struct { const char* cmd; cmdfunc_t func; } cmd_t;
static cmd_t root_commands[] = {
        { "sys", &sys },
        { "wifi", &wifi },
        { "net", &net },
        { NULL, NULL }
};

//=============================================================================
// all sub commands
//=============================================================================
//sys
static void sys_boot(WiFiClient&, String);
static void sys_reboot(WiFiClient&, String);
static void sys_erase_all(WiFiClient&, String);
static cmd_t sys_commands[] = {
        { "boot", &sys_boot },
        { "reboot", &sys_reboot },
        { "erase all", &sys_erase_all },
        { NULL, NULL }
};
//wifi
static void wifi_list(WiFiClient&, String);
static void wifi_add(WiFiClient&, String);
static void wifi_erase(WiFiClient&, String);
static cmd_t wifi_commands[] = {
        { "list", &wifi_list },
        { "add", &wifi_add },
        { "erase", &wifi_erase },
        { NULL, NULL }
};
//net
static void net_hostname(WiFiClient&, String);
static void net_APname(WiFiClient&, String);
static void net_mac(WiFiClient&, String);
static cmd_t net_commands[] = {
        { "hostname", &net_hostname },
        { "APname", &net_APname },
        { "mac", &net_mac },
        { NULL, NULL }
};

//=============================================================================
// common print functions
//=============================================================================
void help(WiFiClient& client, cmd_t* cmd)
{
    client.printf("available sub-commands:\n");
    for(auto i = 0; cmd[i].cmd != NULL; i++){
        client.printf("  %s\n", cmd[i].cmd);
    }
    client.printf("\n");
}
void bad(WiFiClient& client)
{
    client.printf("unknown command\n\n");
}

//=============================================================================
// process incoming buffer passed from telnet function (will be 0 terminated)
//=============================================================================
bool Commander::process(WiFiClient& client, uint8_t* buf)
{
    int end = 0;
    for(; buf[end] >= 32 && buf[end] <= 126; end++); //want only 32-126
    if(not buf[end]) return false; //found 0 terminator
    if(buf[end] != '\r' && buf[end] != '\n') return true; //not cr or lf
    buf[end] = 0; //0 terminate at cr/lf

    String s{(char*)buf};
    s.trim(); //removed leading/trailing space

    //check for root command
    for(auto c : root_commands){
        if(not s.startsWith(c.cmd)) continue;
        s.replace(c.cmd, ""); //remove command string
        c.func(client, s);
        client.printf("$ ");
        return true;
    }

    client.printf("unknown command\n\n$ ");
    return true; //clear buffer and start over
}




//=============================================================================
// all command functions
//=============================================================================
//root commands
void sys(WiFiClient& client, String s)
{
    if(not s[0]){ help(client, sys_commands); return; }
    if(s[0] != ' '){ bad(client); return;  }
    s.trim();
    for(auto c : sys_commands){
        if(not s.startsWith(c.cmd)) continue;
        s.replace(c.cmd, "");
        c.func(client, s);
        return;
    }
    bad(client);
}
void wifi(WiFiClient& client, String s)
{
    if(not s[0]){ help(client, wifi_commands); return; }
    if(s[0] != ' '){ bad(client); return;  }
    s.trim();
    for(auto c : wifi_commands){
        if(not s.startsWith(c.cmd)) continue;
        s.replace(c.cmd, "");
        c.func(client, s);
        return;
    }
    bad(client);
}
void net(WiFiClient& client, String s)
{
    if(not s[0]){ help(client, net_commands); return; }
    if(s[0] != ' '){ bad(client); return;  }
    s.trim();
    for(auto c : net_commands){
        if(not s.startsWith(c.cmd)) continue;
        s.replace(c.cmd, "");
        c.func(client, s);
        return;
    }
    bad(client);
}

//sys
static void sys_boot(WiFiClient& client, String s)
{
    //"sys boot"
    if(not s[0]){
        WifiCredentials wifidata;
        client.printf("boot: %s\n", wifidata.boot() ? "AP" : "STA");
        return;
    }
    //sys boot=AP
    if(s.startsWith("AP")){
        WifiCredentials wifidata;
        wifidata.boot(wifidata.AP);
    }
    //sys boot=STA
    else if(s.startsWith("STA")){
        WifiCredentials wifidata;
        wifidata.boot(wifidata.STA);
        return;
    }
    else bad(client);
}
static void sys_reboot(WiFiClient& client, String s)
{
    //"sys reboot"
    if(s[0]){ bad(client); return; }
    client.printf("rebooting in 5 seconds...");
    delay(5000);
    ESP.restart();
}
static void sys_erase_all(WiFiClient& client, String s)
{
    //"sys erase all"
    if(s[0]){ bad(client); return; }
    client.printf("erasing all stored data...");
    WifiCredentials wifidata;
    wifidata.erase_all();
    client.printf("done.\n");
}
//wifi
static void wifi_list(WiFiClient& client, String s)
{
    //"wifi list"
    if(s[0]){ bad(client); return; }
    WifiCredentials wifidata;
    int max = wifidata.maxn();
    client.printf("[##][ssid][pass]\n\n");
    for( auto i = 0; i < max; i++ ){
        client.printf("[%2d][%s][%s]\n",
            i, wifidata.ssid(i).c_str(), wifidata.pass(i).c_str()
        );
    }
}
static void wifi_add(WiFiClient& client, String s)
{
    //"wifi add"
    if(s[0] != ' '){ bad(client); return;  }
    s = s.substring(1);
    WifiCredentials wifidata;
    //"wifi add 0 ssid=myssid"
    //"wifi add 0 pass=mypass"
    //0 ssid=myssid"
    //0 pass=mypass"
    int idx = 0;
    if(s.substring(0,2) != "0 "){
        idx = s.toInt();
        if(idx == 0){
            client.printf("missing index# or index# not valid\n");
            return;
        }
    }
    if(idx > wifidata.maxn()){
        client.printf("index# is out of range (max %s)\n", wifidata.maxn());
        return;
    }
    int si = s.indexOf("ssid=");
    if(si > 0){
        wifidata.ssid(idx, &s[si+5]);
        return;
    }
    si = s.indexOf("pass=");
    if(si > 0){
        wifidata.pass(idx, &s[si+5]);
        return;
    }
    bad(client);
}
static void wifi_erase(WiFiClient& client, String s)
{
    //"wifi erase"
    if(s[0] != ' '){ bad(client); return; }
    s = s.substring(1);
    //"wifi erase 0"
    int idx = 0;
    if(s.substring(0,1) != "0"){
        idx = s.toInt();
        if(idx == 0){
            client.printf("missing index# or index# not valid\n");
            return;
        }
    }
    WifiCredentials wifidata;
    if(idx > wifidata.maxn()){
        client.printf("index# is out of range\n");
        return;
    }
    wifidata.ssid(idx, "");
    wifidata.pass(idx, "");
}
//net
static void net_hostname(WiFiClient& client, String s)
{
    //"net hostname"
    if(not s[0]){
        WifiCredentials wifidata;
        client.printf("%s\n", WiFi.getHostname());
        return;
    }
    //"net hostname=myname"
    if(s[0] == '='){
        WifiCredentials wifidata;
        s = s.substring(1);
        if(s.length() > 32){
            client.printf("hostname too long (32 chars max)\n");
            return;
        }
        wifidata.hostname(s); //nvs storage
        WiFi.setHostname(s.c_str()); //and out to tcpip (may not see until reboot)
        return;
    }
    bad(client); return;
}
static void net_APname(WiFiClient& client, String s)
{
    //"net APname"
    if(not s[0]){
        WifiCredentials wifidata;
        client.printf("%s\n", wifidata.APname().c_str());
        return;
    }
    //"net APname=myAPname"
    if(s[0] == '='){
        WifiCredentials wifidata;
        s = s.substring(1);
        if(s.length() > 32){
            client.printf("APname too long (32 chars max)\n");
            return;
        }
        wifidata.APname(s); //nvs storage
        return;
    }
    bad(client); return;
}
static void net_mac(WiFiClient& client, String s)
{
    //"net mac"
    if(s[0]){ bad(client); return;  }
    client.printf("%s\n", WiFi.macAddress().c_str());
}

